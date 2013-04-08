/*
 *  appsToAcProtocol.h
 *  
 *
 *  Created by Antonio Davoli on 03/03/09.
 *  Copyright 2009 La Sapienza. All rights reserved.
 *
 */

/* Macro Definition */

/* AC Request Message */

/****************************************
 *	For LIST, QUIT (without argument)	*
 *										*
 *	  	0			    7 				*
 *		+-+-+-+-+-+-+-+-+				*
 *		|    Cmd_msg    |				*
 *		+-+-+-+-+-+-+-+-+				*
 *										*
 ****************************************/ 

/* CMD_MSG Types */

#define FULL_CLIENT_CONNECTED -1
#define CONNECTION_OK 1
#define QUIT_MSG 0
#define LIST_MSG 1
#define CONF_UPDATE_MSG 2

/********************************************************************************************************************************
 * List Response Message:																										*
 *																																*
 *	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   *
 *	|   Active#   |    WTP_ID 1   | NameLength 1  |   WTP Name 1    | ... |    WTP ID N   | NameLength N  |   WTP Name N    |	*
 *	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+	  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+	*
 *																																*
 *	where N is equal to Active# (Number of Active WTPs)																			*
 ********************************************************************************************************************************/


/********************************************************************************************************
 * For CONF_UPDATE_MSG type:																			*
 *																										*
 *	  	0			   7 			   15			   23									 X			*
 *		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+			*
 *		|    cmd_msg    |    msg_elem   |   WPT Index   |		Message Specific Payload	  |			*
 *		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+			*
 *																										*
 ********************************************************************************************************/

#define PAYLOAD_START ((sizeof(unsigned char)*2) + sizeof(int)) 
#define ALL_ACTIVE_WTPS -1

#define MSG_ELEMENT_TYPE_OFDM 1 
#define MSG_ELEMENT_TYPE_VENDOR_UCI 2
#define MSG_ELEMENT_TYPE_VENDOR_WUM 3

/****************************************************************************************
 * Message Specific Payload for MSG_ELEMENT_TYPE_OFDM TYPE (802.11 Binding Version)		* 
 *																						*
 *		0                   1                   2                   3					*
 *		0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1					*
 *		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+				*
 *		|    Radio ID   |    Reserved   | Current Chan  |  Band Support |				*
 *		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+				*
 *		|                         TI Threshold                          |				*
 *		+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+				*
 *																						*										
 ****************************************************************************************/ 

/* Radio ID is filled in the creation message funcion (inside the AC) */




