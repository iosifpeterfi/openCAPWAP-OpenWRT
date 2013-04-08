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

#ifndef __CAPWAP_CWAC_HEADER__
#define __CAPWAP_CWAC_HEADER__

/*_______________________________________________________*/
/*  *******************___INCLUDE___*******************  */
#include "CWCommon.h"
#include "ACMultiHomedSocket.h"
#include "ACProtocol.h"
#include "ACInterface.h"
#include "ACBinding.h"


#include <ctype.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
/*______________________________________________________*/
/*  *******************___DEFINE___*******************  */
#define CW_MAX_WTP				100
#define CW_CRITICAL_TIMER_EXPIRED_SIGNAL	SIGUSR2
#define CW_SOFT_TIMER_EXPIRED_SIGNAL		SIGUSR1
#define AC_LOG_FILE_NAME				"/var/log/ac.log.txt"
#define MAC_ADDR_LEN		6
#define DEST_ADDR_START		4
#define SOURCE_ADDR_START	10

/*Definition of socket's path to send data stats */
#define SOCKET_PATH_AC 				"/tmp/af_unix_ac_client"
#define SOCKET_PATH_RECV_AGENT     		"/tmp/monitorclt"


/*********************************************************
 * 2009 Updates:                                         *
 *				Message Element Types for the CWAsseble- *
 *				ConfigurationUpdateRequest Function.	 *
 *********************************************************/

#define CONFIG_UPDATE_REQ_QOS_ELEMENT_TYPE 0
#define CONFIG_UPDATE_REQ_OFDM_ELEMENT_TYPE 1
#define CONFIG_UPDATE_REQ_VENDOR_UCI_ELEMENT_TYPE 2
#define CONFIG_UPDATE_REQ_VENDOR_WUM_ELEMENT_TYPE 3

/********************************************************
 * 2009 Updates:                                        *
 * For manage the applications connected to AC we use   *
 * an array of socket. Through this array we can set    *
 * easily the correct answer socket.                    * 
 * isFree and numSocketFree are used for management.    *
 * The mutex array is used for serialize the correct	*
 * write operation by the different wtp thread on the	*
 * relative application socket.							*
 ********************************************************/

#define MAX_APPS_CONNECTED_TO_AC 4

typedef struct {
	CWSocket appSocket[MAX_APPS_CONNECTED_TO_AC];
	CWBool isFree[MAX_APPS_CONNECTED_TO_AC];
	CWThreadMutex socketMutex[MAX_APPS_CONNECTED_TO_AC];
	int numSocketFree;
	CWThreadMutex numSocketFreeMutex;
} applicationsManager;

applicationsManager appsManager;

/*_____________________________________________________*/
/*  *******************___TYPES___*******************  */

/* 
 * Struct that describes a WTP from the AC's point of view 
 */
typedef struct {
	CWNetworkLev4Address address;
	CWThread thread;
	CWSecuritySession session;
	CWBool isNotFree;
	CWBool isRequestClose;
	CWStateTransition currentState;
	int interfaceIndex;
	CWSocket socket;
	char buf[CW_BUFFER_SIZE];
	enum  {
		CW_DTLS_HANDSHAKE_IN_PROGRESS,
		CW_WAITING_REQUEST,
		CW_COMPLETED,
	} subState;		
	CWSafeList packetReceiveList;
	
	/* depends on the current state: WaitJoin, NeighborDead */
	CWTimerID currentTimer;
	CWTimerID heartbeatTimer; 
	CWList fragmentsList;
	int pathMTU;
	
	/**** ACInterface ****/
	int interfaceResult;
	CWBool interfaceCommandProgress;
	int interfaceCommand;
	CWThreadMutex interfaceSingleton;
	CWThreadMutex interfaceMutex;
	CWThreadCondition interfaceWait;
	CWThreadCondition interfaceComplete;
	WTPQosValues* qosValues;
	/********************************************************
	 * 2009 Updates:                                        *
	 * - ofdmValues is a struct for setting the values of   *
	 *   a Configuration Update Request with message        *
	 *   elemente OFDM.                                     *
	 * - applicationIndex is the index of application which *
	 *   request the command to WTP                         *
	 *   (it will used for selecting the correct socket)    *
	 ********************************************************/
	
	OFDMControlValues* ofdmValues;  
	CWProtocolVendorSpecificValues* vendorValues;
	int applicationIndex;
	
	/**** ACInterface ****/
	
	CWWTPProtocolManager WTPProtocolManager;
	
	/* Retransmission */
	CWProtocolMessage *messages;
 	int messagesCount;
 	int retransmissionCount;
 	CWTimerID currentPacketTimer;
 	CWBool isRetransmitting;
	
	/* expected response */
	int responseType;
	int responseSeqNum;
	
	/* //Unrecognized message type value
	 * int unrecognizedMsgType;
	 */
	// TAP interface name
	char tap_name[IFNAMSIZ];
	int tap_fd;
	
	// IEEE 802.11
	char RadioInformationABGN;
	char SuppRates[8];
	char MultiDomCapa[6];
	char RadioMAC[6];

} CWWTPManager;	

/*________________________________________________________________*/
/*  *******************___EXTERN VARIABLES___*******************  */
extern CWWTPManager gWTPs[CW_MAX_WTP];
extern CWThreadMutex gWTPsMutex;
extern CWSecurityContext gACSecurityContext;
extern int gACHWVersion;
extern int gACSWVersion;
extern int gActiveStations;
extern int gActiveWTPs;
extern CWThreadMutex gActiveWTPsMutex;
extern int gLimit;
extern int gMaxWTPs;
extern CWAuthSecurity gACDescriptorSecurity;
extern int gRMACField;
extern int gWirelessField;
extern int gDTLSPolicy;
extern CWThreadSpecific gIndexSpecific;
extern char *gACName;
extern int gDiscoveryTimer;
extern int gEchoRequestTimer;
extern int gIdleTimeout;
extern CWProtocolNetworkInterface *gInterfaces;
extern int gInterfacesCount;
extern char **gMulticastGroups;
extern int gMulticastGroupsCount;
extern CWMultiHomedSocket gACSocket;
extern WTPQosValues* gDefaultQosValues;
extern int gHostapd_port;
extern char* gHostapd_unix_path;
extern unsigned char WTPRadioInformationType;



/*________________________________________________________________*/

/*  *******************___GLOBAL VARIABLES FOR ATH_MONITOR TEST___*******************  */
typedef struct {
		struct  sockaddr_un servaddr;/* address of Receive Agent */
		struct  sockaddr_un clntaddr;/* address on AC-side */
		int data_stats_sock;
		} UNIX_SOCKS_INFO;

UNIX_SOCKS_INFO UnixSocksArray[CW_MAX_WTP];

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */

/* in AC.c */
void CWACInit(void);
void CWCreateConnectionWithHostapdAC(void);
void CWACEnterMainLoop(void);
CWBool CWACSendAcknowledgedPacket(int WTPIndex, int msgType, int seqNum);
CWBool CWACResendAcknowledgedPacket(int WTPIndex);
void CWACStopRetransmission(int WTPIndex);
void CWACDestroy(void);

/* in ACTest.h */
CWBool ACQosTest(int WTPIndex);

/* in ACRunState.c */
CWBool CWSaveChangeStateEventRequestMessage(CWProtocolChangeStateEventRequestValues *valuesPtr,
					    CWWTPProtocolManager *WTPProtocolManager);


CW_THREAD_RETURN_TYPE CWACipc_with_ac_hostapd(void *arg);
//send_data_to_hostapd(unsigned char *, int);
/****************************************************************
 * 2009 Updates:												*
 *				msgElement is used for differentiation between	*
 *				all message elements							*
 ****************************************************************/

CWBool CWAssembleConfigurationUpdateRequest(CWProtocolMessage **messagesPtr,
					    int *fragmentsNumPtr,
					    int PMTU,
						int seqNum,
						int msgElement);

CWBool CWAssembleStationConfigurationRequest(CWProtocolMessage **messagesPtr,
					     int *fragmentsNumPtr,
					     int PMTU, int seqNum,
					     unsigned char* StationMacAddr,
					     int Operation); 

CWBool CWAssembleClearConfigurationRequest(CWProtocolMessage **messagesPtr,
					   int *fragmentsNumPtr, int PMTU,
					   int seqNum);

/* in ACDiscoveryState.c */
CWBool CWAssembleDiscoveryResponse(CWProtocolMessage **messagesPtr, int seqNum);
CWBool CWParseDiscoveryRequestMessage(char *msg,
				      int len,
				      int *seqNumPtr,
				      CWDiscoveryRequestValues *valuesPtr);

/* in ACRetransmission.c */
CWBool CWACSendFragments(int WTPIndex);

/* in ACRunStateCheck.c */
CWBool CWACCheckForConfigurationUpdateRequest(int WTPIndex);

/* in ACProtocol_User.c */
CWBool CWACGetVendorInfos(CWACVendorInfos *valPtr);
int CWACGetRMACField();
int CWACGetWirelessField();
int CWACGetDTLSPolicy();
void CWACDestroyVendorInfos(CWACVendorInfos *valPtr);

/* in ACMainLoop.c */
void CWACManageIncomingPacket(CWSocket sock,
			      char *buf,
			      int len,
			      int incomingInterfaceIndex,
			      CWNetworkLev4Address *addrPtr,
			      CWBool dataFlag );

void *CWManageWTP(void *arg);
void CWCloseThread();

/* in CWSecurity.c */
CWBool CWSecurityInitSessionServer(CWWTPManager* pWtp,
				   CWSocket sock,
				   CWSecurityContext ctx,
				   CWSecuritySession *sessionPtr,
				   int *PMTUPtr);

CWBool ACEnterJoin(int WTPIndex, CWProtocolMessage *msgPtr);
CWBool ACEnterConfigure(int WTPIndex, CWProtocolMessage *msgPtr);
CWBool ACEnterDataCheck(int WTPIndex, CWProtocolMessage *msgPtr);
CWBool ACEnterRun(int WTPIndex, CWProtocolMessage *msgPtr, CWBool dataFlag);

CW_THREAD_RETURN_TYPE CWInterface(void* arg);
/* void CWTimerExpiredHandler(int arg); */

#endif
