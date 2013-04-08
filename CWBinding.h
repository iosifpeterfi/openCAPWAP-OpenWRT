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



#ifndef __CAPWAP_CWBinding_HEADER__
#define __CAPWAP_CWBinding_HEADER__

#define CW_BINDING_HLEN				4
#define CW_BINDING_WIRELESSID			1
#define CW_BINDING_DATALENGTH			4

#define NUM_QOS_PROFILES			4
#define UNUSED_QOS_VALUE			255

#define VOICE_QUEUE_INDEX			0
#define VIDEO_QUEUE_INDEX			1
#define BESTEFFORT_QUEUE_INDEX			2
#define BACKGROUND_QUEUE_INDEX			3

#define BINDING_MIN_ELEM_TYPE			1024
#define BINDING_MAX_ELEM_TYPE			2047

#define UNUSED_OFDM_VALUE			255

// Wireless ID viene preso dal campo WBID
//#define CW_TRANSPORT_HEADER_WIRELESS_ID_START	0
//#define CW_TRANSPORT_HEADER_WIRELESS_ID_LEN	8

//#define CW_TRANSPORT_HEADER_LENGTH_START	8
#define CW_TRANSPORT_HEADER_LENGTH_START	0
#define CW_TRANSPORT_HEADER_LENGTH_LEN		8

//#define CW_TRANSPORT_HEADER_RSSI_START		16
#define CW_TRANSPORT_HEADER_RSSI_START		8
#define CW_TRANSPORT_HEADER_RSSI_LEN		8

//#define CW_TRANSPORT_HEADER_SNR_START		24
#define CW_TRANSPORT_HEADER_SNR_START		16
#define CW_TRANSPORT_HEADER_SNR_LEN		8

// Poiche' nel draft 09 il campo del CAPWAP header Wireless Specific 
// Information e' stato privato del sottocampo Wireless ID con il
// conseguente shift a sx di 8 bit dei sottocampi successivi il sottocampo
// datarate del binding si trova a cavallo tra 2 word da 4 byte quindi
// vanno specificati due offset.
//#define CW_TRANSPORT_HEADER_DATARATE_START	0
//#define CW_TRANSPORT_HEADER_DATARATE_LEN	16

#define CW_TRANSPORT_HEADER_DATARATE_1_START	24
#define CW_TRANSPORT_HEADER_DATARATE_1_LEN	8

#define CW_TRANSPORT_HEADER_DATARATE_2_START	0
#define CW_TRANSPORT_HEADER_DATARATE_2_LEN	8

//#define CW_TRANSPORT_HEADER_PADDING_START	16
//#define CW_TRANSPORT_HEADER_PADDING_LEN		16
#define CW_TRANSPORT_HEADER_PADDING_START	8
#define CW_TRANSPORT_HEADER_PADDING_LEN		24

#define BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL	1033
#define BINDING_MSG_ELEMENT_TYPE_WTP_QOS	1045

/****************************************************
 * 2009 Update:										*
 *				Lengths Messages Element Definition *
 *				Specified in the 802.11 binding		*
 ****************************************************/

#define BINDING_MSG_ELEMENT_TYPE_OFDM_CONTROL_LENGTH 8

/****************************************************
 * 2009 Update:										*
 *				Define Structure for OFDM contol	*
 *				Values Management					*
 ****************************************************/

typedef struct
{
  int currentChan; /* 16 Bit Value */
  unsigned char BandSupport;
  unsigned int TIThreshold;
} OFDMControlValues;

typedef struct
{
	unsigned char queueDepth;
	int cwMin;
	int cwMax;
	unsigned char  AIFS;
	unsigned char dot1PTag;
	unsigned char DSCPTag;	
} WTPQosValues;

typedef struct
{
	WTPQosValues* qosValues;
} bindingValues;

/*---------------------------*/

typedef struct {
	char RSSI;
	char SNR;
	int dataRate;
} CWBindingTransportHeaderValues;

typedef struct {
	CWProtocolMessage* frame;
	CWBindingTransportHeaderValues* bindingValues;
} CWBindingDataListElement;

extern const int gMaxCAPWAPHeaderSizeBinding;

CWBool CWAssembleDataMessage(CWProtocolMessage **completeMsgPtr, int *fragmentsNumPtr, int PMTU, CWProtocolMessage *frame, CWBindingTransportHeaderValues *bindingValuesPtr, int is_crypted, int keepAlive);
CWBool CWAssembleTransportHeaderBinding(CWProtocolMessage *transportHdrPtr, CWBindingTransportHeaderValues *valuesPtr);
CWBool CWBindingCheckType(int elemType);
CWBool CWParseTransportHeaderBinding(CWProtocolMessage *msgPtr, CWBindingTransportHeaderValues *valuesPtr);
CWBool CWParseTransportHeaderMACAddress(CWProtocolMessage *msgPtr, char *mac_ptr);

#endif
