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


#ifndef __CAPWAP_CWProtocol_HEADER__
#define __CAPWAP_CWProtocol_HEADER__

//#define CWSetField32(obj, start, val)	((obj)[start/64]) |= ((val) << (start%64))	
//#define CWGetField32(obj, start, len)	(((obj)[start/64]) & ((0xFFFFFFFFFFFFFFFF >> (64-(len))) << (start%64)) ) >> (start%64)

/*_____________________________________________________*/
/*  *******************___MACRO___*******************  */
//#define CWSetField32(obj, start, val)					((obj)[start/32]) |= ((val) << (start%32))	
//#define CWGetField32(obj, start, len)					(((obj)[start/32]) & ((0xFFFFFFFFUL >> (32-(len))) << (start%32)) ) >> (start%32)

#define CWSetField32(src,start,len,val)					src |= ((~(0xFFFFFFFF << len)) & val) << (32 - start - len)
#define CWGetField32(src,start,len)					((~(0xFFFFFFFF<<len)) & (src >> (32 - start - len)))

#define CW_REWIND_BYTES(buf, bytes, type)				(buf) = (type*)(((char*) (buf)) - bytes)
#define CW_SKIP_BYTES(buf, bytes, type)					(buf) = (type*)(((char*) (buf)) + bytes)
#define CW_SKIP_BITS(buf, bits, type)					(buf) = (type*)(((char*) (buf)) + ((bits) / 8))
#define CW_BYTES_TO_STORE_BITS(bits)					((((bits) % 8) == 0) ? ((bits) / 8) : (((bits) / 8)+1))

#define		CW_CREATE_PROTOCOL_MESSAGE(mess, size, err)		CW_CREATE_OBJECT_SIZE_ERR(((mess).msg), (size), err);		\
									CW_ZERO_MEMORY(((mess).msg), (size));						\
									(mess).offset = 0;

#define 	CW_CREATE_PROTOCOL_MSG_ARRAY_ERR(ar_name, ar_size, on_err) 	{\
											CW_CREATE_ARRAY_ERR(ar_name, ar_size, CWProtocolMessage, on_err)\
											int i;\
											for(i=0;i<(ar_size); i++) {\
												(ar_name)[i].msg = NULL;\
												(ar_name)[i].offset = 0; \
											}\
										}

#define		CW_FREE_PROTOCOL_MESSAGE(mess)				CW_FREE_OBJECT(((mess).msg));								\
									(mess).offset = 0;
															
#define		CWParseMessageElementStart()				int oldOffset;												\
									if(msgPtr == NULL || valPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);	\
									oldOffset = msgPtr->offset
						
#define		CWParseMessageElementEnd()				CWDebugLog(NULL);											\
									return ((msgPtr->offset) - oldOffset) == len ? CW_TRUE :	\
									CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Message Element Malformed");


/*_________________________________________________________*/
/*  *******************___CONSTANTS___*******************  */

// to be defined
#define     	MAX_UDP_PACKET_SIZE					65536
#define		CW_CONTROL_PORT						5246
#define		CW_DATA_PORT						5247
#define		CW_PROTOCOL_VERSION					0
#define		CW_IANA_ENTERPRISE_NUMBER				13277	
#define		CW_IANA_ENTERPRISE_NUMBER_VENDOR_IANA			18357
#define 	CW_CONTROL_HEADER_OFFSET_FOR_MSG_ELEMS			3		//Offset "Seq Num" - "Message Elements"
#define		CW_MAX_SEQ_NUM						255
#define 	CW_MAX_FRAGMENT_ID					65535
#define 	CLEAR_DATA						1
#define		DTLS_ENABLED_DATA					2
#define		CW_PACKET_PLAIN						0
#define		CW_PACKET_CRYPT						1
#define 	CW_DATA_MSG_FRAME_TYPE					1
#define		CW_DATA_MSG_STATS_TYPE					2
#define     CW_DATA_MSG_FREQ_STATS_TYPE             3 /* 2009 Update */
#define     	CW_IEEE_802_3_FRAME_TYPE				4
#define     	CW_DATA_MSG_KEEP_ALIVE_TYPE				5

#define     	CW_IEEE_802_11_FRAME_TYPE				6

// <TRANSPORT_HEADER_FIELDS>
// CAPWAP version (currently 0)
#define 	CW_TRANSPORT_HEADER_VERSION_START			0
#define 	CW_TRANSPORT_HEADER_VERSION_LEN				4

// Mauro
#define		CW_TRANSPORT_HEADER_TYPE_START				4
#define		CW_TRANSPORT_HEADER_TYPE_LEN				4

// Radio ID number (for WTPs with multiple radios)
#define 	CW_TRANSPORT_HEADER_RID_START				13
#define 	CW_TRANSPORT_HEADER_RID_LEN				5

// Length of CAPWAP tunnel header in 4 byte words 
#define 	CW_TRANSPORT_HEADER_HLEN_START				8
#define 	CW_TRANSPORT_HEADER_HLEN_LEN				5

// Wireless Binding ID
#define 	CW_TRANSPORT_HEADER_WBID_START				18
#define 	CW_TRANSPORT_HEADER_WBID_LEN				5

// Format of the frame
#define 	CW_TRANSPORT_HEADER_T_START				23
#define 	CW_TRANSPORT_HEADER_T_LEN				1

// Is a fragment?
#define 	CW_TRANSPORT_HEADER_F_START				24
#define 	CW_TRANSPORT_HEADER_F_LEN				1

// Is NOT the last fragment?
#define 	CW_TRANSPORT_HEADER_L_START				25
#define 	CW_TRANSPORT_HEADER_L_LEN				1

// Is the Wireless optional header present?
#define 	CW_TRANSPORT_HEADER_W_START				26
#define 	CW_TRANSPORT_HEADER_W_LEN				1

// Is the Radio MAC Address optional field present?
#define 	CW_TRANSPORT_HEADER_M_START				27
#define 	CW_TRANSPORT_HEADER_M_LEN				1

// Is the message a keep alive?
#define 	CW_TRANSPORT_HEADER_K_START				28
#define 	CW_TRANSPORT_HEADER_K_LEN				1

// Set to 0 in this version of the protocol
#define 	CW_TRANSPORT_HEADER_FLAGS_START				29
#define 	CW_TRANSPORT_HEADER_FLAGS_LEN				3

// ID of the group of fragments
#define 	CW_TRANSPORT_HEADER_FRAGMENT_ID_START			0
#define 	CW_TRANSPORT_HEADER_FRAGMENT_ID_LEN			16

// Position of this fragment in the group 
#define 	CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_START		16
#define 	CW_TRANSPORT_HEADER_FRAGMENT_OFFSET_LEN			13

// Set to 0 in this version of the protocol
#define 	CW_TRANSPORT_HEADER_RESERVED_START			29
#define 	CW_TRANSPORT_HEADER_RESERVED_LEN			3
// </TRANSPORT_HEADER_FIELDS>


// Message Type Values
#define		CW_MSG_TYPE_VALUE_DISCOVERY_REQUEST			1
#define		CW_MSG_TYPE_VALUE_DISCOVERY_RESPONSE			2
#define		CW_MSG_TYPE_VALUE_JOIN_REQUEST				3
#define		CW_MSG_TYPE_VALUE_JOIN_RESPONSE				4
#define		CW_MSG_TYPE_VALUE_CONFIGURE_REQUEST			5
#define		CW_MSG_TYPE_VALUE_CONFIGURE_RESPONSE			6
#define		CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_REQUEST		7
#define		CW_MSG_TYPE_VALUE_CONFIGURE_UPDATE_RESPONSE		8
#define 	CW_MSG_TYPE_VALUE_WTP_EVENT_REQUEST			9
#define 	CW_MSG_TYPE_VALUE_WTP_EVENT_RESPONSE			10
#define		CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_REQUEST		11
#define		CW_MSG_TYPE_VALUE_CHANGE_STATE_EVENT_RESPONSE		12
#define		CW_MSG_TYPE_VALUE_ECHO_REQUEST				13
#define		CW_MSG_TYPE_VALUE_ECHO_RESPONSE				14
#define		CW_MSG_TYPE_VALUE_IMAGE_DATA_REQUEST			15
#define		CW_MSG_TYPE_VALUE_IMAGE_DATA_RESPONSE			16
#define		CW_MSG_TYPE_VALUE_RESET_REQUEST				17
#define		CW_MSG_TYPE_VALUE_RESET_RESPONSE			18
#define		CW_MSG_TYPE_VALUE_PRIMARY_DISCOVERY_REQUEST		19
#define		CW_MSG_TYPE_VALUE_PRIMARY_DISCOVERY_RESPONSE		20
#define		CW_MSG_TYPE_VALUE_DATA_TRANSFER_REQUEST			21
#define		CW_MSG_TYPE_VALUE_DATA_TRANSFER_RESPONSE		22
#define		CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_REQUEST		23
#define		CW_MSG_TYPE_VALUE_CLEAR_CONFIGURATION_RESPONSE		24
#define		CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_REQUEST		25
#define		CW_MSG_TYPE_VALUE_STATION_CONFIGURATION_RESPONSE	26

// IEEE 802.11 Binding Type
#define		CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_REQUEST		3398913
#define		CW_MSG_TYPE_VALUE_WLAN_CONFIGURATION_RESPONSE		3398914

// Message Elements Type Values
#define 	CW_MSG_ELEMENT_AC_DESCRIPTOR_CW_TYPE			1
#define 	CW_MSG_ELEMENT_AC_IPV4_LIST_CW_TYPE			2
#define 	CW_MSG_ELEMENT_AC_IPV6_LIST_CW_TYPE			3
#define 	CW_MSG_ELEMENT_AC_NAME_CW_TYPE				4
#define 	CW_MSG_ELEMENT_AC_NAME_INDEX_CW_TYPE			5
#define		CW_MSG_ELEMENT_TIMESTAMP_CW_TYPE			6
#define		CW_MSG_ELEMENT_ADD_MAC_ACL_CW_TYPE			7
#define		CW_MSG_ELEMENT_ADD_STATION_CW_TYPE			8
#define		CW_MSG_ELEMENT_ADD_STATIC_MAC_ACL_CW_TYPE		9
#define 	CW_MSG_ELEMENT_CW_CONTROL_IPV4_ADDRESS_CW_TYPE		10
#define 	CW_MSG_ELEMENT_CW_CONTROL_IPV6_ADDRESS_CW_TYPE		11
#define		CW_MSG_ELEMENT_CW_TIMERS_CW_TYPE			12
#define		CW_MSG_ELEMENT_DATA_TRANSFER_DATA_CW_TYPE		13
#define		CW_MSG_ELEMENT_DATA_TRANSFER_MODE_CW_TYPE		14
#define 	CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_CW_TYPE		15
#define 	CW_MSG_ELEMENT_CW_DECRYPT_ER_REPORT_PERIOD_CW_TYPE	16
#define 	CW_MSG_ELEMENT_DELETE_MAC_ACL_CW_TYPE			17
#define 	CW_MSG_ELEMENT_DELETE_STATION_CW_TYPE			18
#define 	CW_MSG_ELEMENT_DELETE_STATIC_MAC_ACL_CW_TYPE		19
#define 	CW_MSG_ELEMENT_DISCOVERY_TYPE_CW_TYPE			20
#define 	CW_MSG_ELEMENT_DUPLICATE_IPV4_ADDRESS_CW_TYPE		21
#define 	CW_MSG_ELEMENT_DUPLICATE_IPV6_ADDRESS_CW_TYPE		22
#define 	CW_MSG_ELEMENT_IDLE_TIMEOUT_CW_TYPE			23
#define 	CW_MSG_ELEMENT_IMAGE_DATA_CW_TYPE			24
#define 	CW_MSG_ELEMENT_IMAGE_IDENTIFIER_CW_TYPE			25
#define 	CW_MSG_ELEMENT_IMAGE_INFO_CW_TYPE			26
#define 	CW_MSG_ELEMENT_INITIATED_DOWNLOAD_CW_TYPE		27
#define 	CW_MSG_ELEMENT_LOCATION_DATA_CW_TYPE			28
#define 	CW_MSG_ELEMENT_MAX_MSG_LEN_CW_TYPE			29
#define 	CW_MSG_ELEMENT_WTP_IPV4_ADDRESS_CW_TYPE			30
#define 	CW_MSG_ELEMENT_RADIO_ADMIN_STATE_CW_TYPE		31
#define 	CW_MSG_ELEMENT_RADIO_OPERAT_STATE_CW_TYPE		32
#define 	CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE			33
#define 	CW_MSG_ELEMENT_RETURNED_MSG_ELEM_CW_TYPE		34
#define 	CW_MSG_ELEMENT_SESSION_ID_CW_TYPE			35
#define 	CW_MSG_ELEMENT_STATISTICS_TIMER_CW_TYPE			36
#define 	CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_BW_CW_TYPE		37
#define 	CW_MSG_ELEMENT_WTP_BOARD_DATA_CW_TYPE			38
#define 	CW_MSG_ELEMENT_WTP_DESCRIPTOR_CW_TYPE			39
#define 	CW_MSG_ELEMENT_WTP_FALLBACK_CW_TYPE			40
#define 	CW_MSG_ELEMENT_WTP_FRAME_TUNNEL_MODE_CW_TYPE		41
#define 	CW_MSG_ELEMENT_WTP_MAC_TYPE_CW_TYPE			44
#define 	CW_MSG_ELEMENT_WTP_NAME_CW_TYPE				45
#define 	CW_MSG_ELEMENT_WTP_OPERAT_STATISTICS_CW_TYPE		46
#define 	CW_MSG_ELEMENT_WTP_RADIO_STATISTICS_CW_TYPE		47
#define 	CW_MSG_ELEMENT_WTP_REBOOT_STATISTICS_CW_TYPE		48
#define 	CW_MSG_ELEMENT_WTP_STATIC_IP_CW_TYPE			49
/*Update 2009:
		Message type to return a payload together with the 
		configuration update response*/
#define 	CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_CW_TYPE		57
#define 	CW_MSG_ELEMENT_RESULT_CODE_CW_TYPE_WITH_PAYLOAD		58
#define 	CW_MSG_ELEMENT_MTU_DISCOVERY_PADDING_CW_TYPE		59

//Vendor Specific data
#define 	CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_AC_PRIORITY		128
#define 	CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_AC_PRIORITY_LEN	1
#define 	CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_AC_PRIORITY_DEFAULT	3

// IEEE 802.11 Message Element
#define 	CW_MSG_ELEMENT_IEEE80211_ADD_WLAN_CW_TYPE						1024
#define 	CW_MSG_ELEMENT_IEEE80211_DELETE_WLAN_CW_TYPE					1027
#define 	CW_MSG_ELEMENT_IEEE80211_MULTI_DOMAIN_CAPABILITY_CW_TYPE		1032
#define 	CW_MSG_ELEMENT_IEEE80211_SUPPORTED_RATES_CW_TYPE				1040
#define 	CW_MSG_ELEMENT_IEEE80211_WTP_RADIO_INFORMATION_CW_TYPE			1048

// CAPWAP Protocol Variables
#define		CW_MAX_RETRANSMIT_DEFAULT		5
#define 	CW_WAIT_JOIN_DEFAULT			60
#define		CW_REPORT_INTERVAL_DEFAULT		120
#define		CW_STATISTIC_TIMER_DEFAULT		120

#ifdef CW_DEBUGGING
	#define		CW_JOIN_INTERVAL_DEFAULT 	60
#else
	#define		CW_JOIN_INTERVAL_DEFAULT 	60
#endif

#ifdef CW_DEBUGGING
	#define		CW_CHANGE_STATE_INTERVAL_DEFAULT 10
#else
	#define		CW_CHANGE_STATE_INTERVAL_DEFAULT 25
#endif

#ifdef CW_DEBUGGING
	#define 	CW_RETRANSMIT_INTERVAL_DEFAULT		12
#else
	#define 	CW_RETRANSMIT_INTERVAL_DEFAULT		3
#endif

#ifdef CW_DEBUGGING
	#define		CW_NEIGHBORDEAD_INTERVAL_DEFAULT	70
	#define		CW_NEIGHBORDEAD_RESTART_DISCOVERY_DELTA_DEFAULT	((CW_NEIGHBORDEAD_INTERVAL_DEFAULT) + 40)
#else
	#define		CW_NEIGHBORDEAD_INTERVAL_DEFAULT	70
	#define		CW_NEIGHBORDEAD_RESTART_DISCOVERY_DELTA_DEFAULT	((CW_NEIGHBORDEAD_INTERVAL_DEFAULT) + 40)
#endif

#ifdef CW_DEBUGGING
	#define		CW_ECHO_INTERVAL_DEFAULT		2
	#define		CW_DATA_CHANNEL_KEEP_ALIVE_DEFAULT	2
#else
	#define		CW_ECHO_INTERVAL_DEFAULT		30
	#define		CW_DATA_CHANNEL_KEEP_ALIVE_DEFAULT	30
#endif


/*_________________________________________________________*/
/*  *******************___VARIABLES___*******************  */

/*_____________________________________________________*/
/*  *******************___TYPES___*******************  */
typedef struct {
	int type;
	int value;
} CWMsgElemData;

typedef unsigned char CWMACAddress[6];

typedef enum {
	CW_PROTOCOL_SUCCESS				= 0, //	Success
	CW_PROTOCOL_FAILURE_AC_LIST			= 1, // AC List message MUST be present
	CW_PROTOCOL_SUCCESS_NAT				= 2, // NAT detected
	CW_PROTOCOL_FAILURE				= 3, // unspecified
	CW_PROTOCOL_FAILURE_RES_DEPLETION		= 4, // Resource Depletion
	CW_PROTOCOL_FAILURE_UNKNOWN_SRC			= 5, // Unknown Source
	CW_PROTOCOL_FAILURE_INCORRECT_DATA		= 6, // Incorrect Data
	CW_PROTOCOL_FAILURE_ID_IN_USE			= 7, // Session ID Alreadyin Use
	CW_PROTOCOL_FAILURE_WTP_HW_UNSUPP		= 8, // WTP Hardware not supported
	CW_PROTOCOL_FAILURE_BINDING_UNSUPP		= 9, // Binding not supported
	CW_PROTOCOL_FAILURE_UNABLE_TO_RESET		= 10, // Unable to reset
	CW_PROTOCOL_FAILURE_FIRM_WRT_ERROR		= 11, // Firmware write error
	CW_PROTOCOL_FAILURE_SERVICE_PROVIDED_ANYHOW	= 12, // Unable to apply requested configuration 
	CW_PROTOCOL_FAILURE_SERVICE_NOT_PROVIDED	= 13, // Unable to apply requested configuration
	CW_PROTOCOL_FAILURE_INVALID_CHECKSUM		= 14, // Image Data Error: invalid checksum
	CW_PROTOCOL_FAILURE_INVALID_DATA_LEN		= 15, // Image Data Error: invalid data length
	CW_PROTOCOL_FAILURE_OTHER_ERROR			= 16, // Image Data Error: other error
	CW_PROTOCOL_FAILURE_IMAGE_ALREADY_PRESENT	= 17, // Image Data Error: image already present
	CW_PROTOCOL_FAILURE_INVALID_STATE		= 18, // Message unexpected: invalid in current state
	CW_PROTOCOL_FAILURE_UNRECOGNIZED_REQ		= 19, // Message unexpected: unrecognized request
	CW_PROTOCOL_FAILURE_MISSING_MSG_ELEM		= 20, // Failure: missing mandatory message element
	CW_PROTOCOL_FAILURE_UNRECOGNIZED_MSG_ELEM	= 21  // Failure: unrecognized message element


} CWProtocolResultCode;

typedef struct {
	char *msg;
	int offset;
	int data_msgType; 
} CWProtocolMessage;


#define 	MAX_PENDING_REQUEST_MSGS	15
#define		UNUSED_MSG_TYPE			0

typedef struct {
	unsigned char msgType;
	unsigned char seqNum;
	int timer_sec;
	void (*timer_hdl)(void *);
	CWTimerArg timer_arg;
	CWTimerID timer;
	int retransmission;
	CWProtocolMessage *msgElems;
	int fragmentsNum;
} CWPendingRequestMessage;

#include "CWBinding.h"

typedef struct {
	int payloadType;
	int type;
	int isFragment;
	int last;
	int fragmentID;
	int fragmentOffset;
	int keepAlive;
	CWBindingTransportHeaderValues *bindingValuesPtr;
} CWProtocolTransportHeaderValues;

typedef struct {
	unsigned int messageTypeValue;
	unsigned char seqNum;
	unsigned short msgElemsLen;
//	unsigned int timestamp;
} CWControlHeaderValues;

typedef struct {
	char *data;
	int dataLen;
	CWProtocolTransportHeaderValues transportVal;
} CWProtocolFragment;

typedef struct {
	int vendorIdentifier;
	enum {
		CW_WTP_MODEL_NUMBER	= 0,
		CW_WTP_SERIAL_NUMBER	= 1,
		CW_BOARD_ID		= 2,
		CW_BOARD_REVISION	= 3,

		CW_WTP_HARDWARE_VERSION	= 0,
		CW_WTP_SOFTWARE_VERSION	= 1,
		CW_BOOT_VERSION		= 2
	} type;
	int length;
	char *valuePtr;
} CWWTPVendorInfoValues;

typedef struct  {
	int vendorInfosCount;
	CWWTPVendorInfoValues *vendorInfos;
} CWWTPVendorInfos;

typedef struct {
	int maxRadios;
	int radiosInUse;
	int encCapabilities;
	CWWTPVendorInfos vendorInfos;
} CWWTPDescriptor;

typedef enum {
	CW_RESERVE = 1,
	CW_LOCAL_BRIDGING = 2,
	CW_802_DOT_3_BRIDGING = 4,
	CW_NATIVE_BRIDGING = 8,
	CW_ALL_ENC = 15
} CWframeTunnelMode;

typedef enum {
	CW_LOCAL_MAC = 0,
	CW_SPLIT_MAC = 1,
	CW_BOTH = 2
} CWMACType;

typedef struct {
	enum {
		CW_MSG_ELEMENT_DISCOVERY_TYPE_BROADCAST = 0,
		CW_MSG_ELEMENT_DISCOVERY_TYPE_CONFIGURED = 1
	} type;
	CWWTPVendorInfos WTPBoardData;
	CWWTPDescriptor WTPDescriptor;
	CWframeTunnelMode frameTunnelMode;
	CWMACType MACType;

} CWDiscoveryRequestValues;

typedef enum {
	CW_X509_CERTIFICATE = 1,
	CW_PRESHARED = 0
} CWAuthSecurity;

typedef struct {
	CWNetworkLev4Address addr;
	struct sockaddr_in addrIPv4;

	int WTPCount;
} CWProtocolNetworkInterface;

typedef struct {
	int WTPCount;
	struct sockaddr_in addr;
} CWProtocolIPv4NetworkInterface;

typedef struct {
	int WTPCount;
	struct sockaddr_in6 addr;
} CWProtocolIPv6NetworkInterface;

typedef struct {
	int vendorIdentifier;
	enum {
		CW_AC_HARDWARE_VERSION	= 4,
		CW_AC_SOFTWARE_VERSION	= 5
	} type;
	int length;
	int *valuePtr;
} CWACVendorInfoValues;

typedef struct  {
	int vendorInfosCount;
	CWACVendorInfoValues *vendorInfos;
} CWACVendorInfos;

typedef struct {
	int rebootCount;
	int ACInitiatedCount;
	int linkFailurerCount;
	int SWFailureCount;
	int HWFailuireCount;
	int otherFailureCount;
	int unknownFailureCount;
	enum {
		NOT_SUPPORTED=0,
		AC_INITIATED=1,
		LINK_FAILURE=2,
		SW_FAILURE=3,
		HD_FAILURE=4,
		OTHER_FAILURE=5,
		UNKNOWN=255
	}lastFailureType;
}WTPRebootStatisticsInfo;

typedef struct
{
	int radioID;
	int reportInterval;
}WTPDecryptErrorReportValues;

typedef struct
{
	int radiosCount;
	WTPDecryptErrorReportValues *radios;
}WTPDecryptErrorReport;

typedef struct {
	int index;
	char *ACName;
}CWACNameWithIndexValues;

typedef struct {
	int count;
	CWACNameWithIndexValues *ACNameIndex;
}CWACNamesWithIndex;

typedef struct {
	int ID;
	enum {
		CW_802_DOT_11b = 1,
		CW_802_DOT_11a = 2,
		CW_802_DOT_11g = 4,
		CW_802_DOT_11n = 8,
		CW_ALL_RADIO_TYPES = 0x0F
	} type;
} CWRadioInformationValues;

typedef struct {
	int radiosCount;
	CWRadioInformationValues *radios;
} CWRadiosInformation;

typedef enum {
	ENABLED = 1,
	DISABLED = 2
} CWstate;

typedef enum {
	AD_NORMAL = 1,
	AD_RADIO_FAILURE = 2,
	AD_SOFTWARE_FAILURE = 3,
	AD_RADAR_DETECTION = 4
} CWAdminCause;

typedef enum {
	OP_NORMAL = 0,
	OP_RADIO_FAILURE = 1,
	OP_SOFTWARE_FAILURE = 2,
	OP_ADMINISTRATIVELY_SET = 3
} CWOperationalCause;

typedef struct {
	int ID;
	CWstate state;
	CWAdminCause cause;
} CWRadioAdminInfoValues;

typedef struct {
	int radiosCount;
	CWRadioAdminInfoValues *radios;
} CWRadiosAdminInfo;

typedef struct {
	int ID;
	CWstate state;
	CWOperationalCause cause;
} CWRadioOperationalInfoValues;

typedef struct {
	int radiosCount;
	CWRadioOperationalInfoValues *radios;
} CWRadiosOperationalInfo;

typedef struct {
	int ID;
	unsigned char numEntries;
	unsigned char length;
	CWMACAddress *decryptErrorMACAddressList;
} CWDecryptErrorReportValues;

typedef struct {
	int radiosCount;
	CWDecryptErrorReportValues *radios;
} CWDecryptErrorReportInfo;

typedef struct {
	enum {
		STATISTICS_NOT_SUPPORTED=0,
		SW_FAILURE_TYPE=1,
		HD_FAILURE_TYPE=2,
		OTHER_TYPES=3,
		UNKNOWN_TYPE=255
	}lastFailureType;
	short int resetCount;
	short int SWFailureCount;
	short int HWFailuireCount;
	short int otherFailureCount;
	short int unknownFailureCount;
	short int configUpdateCount;
	short int channelChangeCount;
	short int bandChangeCount;
	short int currentNoiseFloor;
}WTPRadioStatisticsInfo;

typedef struct {
	unsigned int radioID;
	//Station Mac Address List

	CWList decryptErrorMACAddressList;

	unsigned int reportInterval;
	
	CWstate adminState;
	CWAdminCause adminCause;

	CWstate operationalState;
	CWOperationalCause operationalCause;

	unsigned int TxQueueLevel;
	unsigned int wirelessLinkFramesPerSec;

	WTPRadioStatisticsInfo statistics;	
	
	void* bindingValuesPtr;
} CWWTPRadioInfoValues;

typedef struct {
	int radioCount;
	CWWTPRadioInfoValues *radiosInfo;
} CWWTPRadiosInfo;

/*Update 2009:
	Helper structure to keep track of 
	requested UCI commands (via Vendor specific
	message)*/
typedef struct {
	unsigned short vendorPayloadType;
	void *payload;
} CWProtocolVendorSpecificValues;

#include "CWList.h"

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */

__inline__ unsigned int CWGetSeqNum(); // provided by the user of CWProtocol lib
__inline__ int CWGetFragmentID(); // provided by the user of CWProtocol lib

void CWWTPResetRadioStatistics(WTPRadioStatisticsInfo *radioStatistics);

void CWProtocolDestroyMsgElemData(void *f);
void CWFreeMessageFragments(CWProtocolMessage* messages, int fragmentsNum);

void CWProtocolStore8(CWProtocolMessage *msgPtr, unsigned char val);
void CWProtocolStore16(CWProtocolMessage *msgPtr, unsigned short val);
void CWProtocolStore32(CWProtocolMessage *msgPtr, unsigned int val);
void CWProtocolStoreStr(CWProtocolMessage *msgPtr, char *str);
void CWProtocolStoreMessage(CWProtocolMessage *msgPtr, CWProtocolMessage *msgToStorePtr);
void CWProtocolStoreRawBytes(CWProtocolMessage *msgPtr, char *bytes, int len);

unsigned char CWProtocolRetrieve8(CWProtocolMessage *msgPtr);
unsigned short CWProtocolRetrieve16(CWProtocolMessage *msgPtr);
unsigned int CWProtocolRetrieve32(CWProtocolMessage *msgPtr);
char *CWProtocolRetrieveStr(CWProtocolMessage *msgPtr, int len);
char *CWProtocolRetrieveRawBytes(CWProtocolMessage *msgPtr, int len);

CWBool CWProtocolParseFragment(char *buf, int readBytes, CWList *fragmentsListPtr, CWProtocolMessage *reassembledMsg, CWBool *dataFlag, char *RadioMAC);
void CWProtocolDestroyFragment(void *f);

CWBool CWParseTransportHeader(CWProtocolMessage *msgPtr, CWProtocolTransportHeaderValues *valuesPtr, CWBool *dataFlag, char *RadioMAC);
CWBool CWParseControlHeader(CWProtocolMessage *msgPtr, CWControlHeaderValues *valPtr);
CWBool CWParseFormatMsgElem(CWProtocolMessage *completeMsg,unsigned short int *type,unsigned short int *len);

CWBool CWAssembleTransportHeader(CWProtocolMessage *transportHdrPtr, CWProtocolTransportHeaderValues *valuesPtr);
CWBool CWAssembleTransportHeaderKeepAliveData(CWProtocolMessage *transportHdrPtr, CWProtocolTransportHeaderValues *valuesPtr, int keepAlive);
CWBool CWAssembleControlHeader(CWProtocolMessage *controlHdrPtr, CWControlHeaderValues *valPtr);
CWBool CWAssembleMessage(CWProtocolMessage **completeMsgPtr, int *fragmentsNumPtr, int PMTU, int seqNum, int msgTypeValue, CWProtocolMessage *msgElems, const int msgElemNum, CWProtocolMessage *msgElemsBinding, const int msgElemBindingNum, int is_crypted);
CWBool CWAssembleMsgElem(CWProtocolMessage *msgPtr, unsigned int type);
CWBool CWAssembleUnrecognizedMessageResponse(CWProtocolMessage **messagesPtr, int *fragmentsNumPtr, int PMTU, int seqNum, int msgType);

CWBool CWAssembleMsgElemRadioAdminState(CWProtocolMessage *msgPtr);			//29
CWBool CWAssembleMsgElemRadioOperationalState(int radioID, CWProtocolMessage *msgPtr);	//30
CWBool CWAssembleMsgElemResultCode(CWProtocolMessage *msgPtr,CWProtocolResultCode code);//31
CWBool CWAssembleVendorMsgElemResultCodeWithPayload(CWProtocolMessage *msgPtr,CWProtocolResultCode code, CWProtocolVendorSpecificValues *payload);//49
CWBool CWAssembleMsgElemSessionID(CWProtocolMessage *msgPtr, char * sessionID);		//32

CWBool CWParseACName(CWProtocolMessage *msgPtr, int len, char **valPtr);
CWBool CWParseWTPRadioOperationalState (CWProtocolMessage *msgPtr, int len, CWRadioOperationalInfoValues *valPtr);	//30
CWBool CWParseResultCode(CWProtocolMessage *msgPtr, int len, CWProtocolResultCode *valPtr);			//31
char * CWParseSessionID(CWProtocolMessage *msgPtr, int len);

#endif
