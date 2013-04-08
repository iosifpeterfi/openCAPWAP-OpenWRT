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

#ifndef __CAPWAP_WTPFrameReceive_HEADER__
#define __CAPWAP_WTPFrameReceive_HEADER__

#include <sys/socket.h>
#include <sys/types.h>  
#include <linux/if_ether.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>

#include "CWWTP.h"

#ifndef B_ENDIAN
#define CHECK_PRISM_HEADER	(buffer[0]==MESSAGE_CODE_1) && \
 				(buffer[1]==MESSAGE_CODE_2) && \
 				(buffer[2]==MESSAGE_CODE_3) && \
 				(buffer[3]==MESSAGE_CODE_4)
#else
#define CHECK_PRISM_HEADER	(buffer[3]==MESSAGE_CODE_1) && \
				(buffer[2]==MESSAGE_CODE_2) && \
				(buffer[1]==MESSAGE_CODE_3) && \
				(buffer[0]==MESSAGE_CODE_4)
#endif

//#define PROMODE_ON
//#define FILTER_ON
#define PRISMH_LEN		144
#define RSSI_BYTE		68
#define SIGNAL_BYTE		92
#define NOISE_BYTE		104
#define DATARATE_BYTE		116
#define DEST_ADDR_START		4
#define SOURCE_ADDR_START	10
#define MESSAGE_CODE_1		68
#define MESSAGE_CODE_2		0
#define MESSAGE_CODE_3		0
#define MESSAGE_CODE_4		0
#define MAC_ADDR_LEN		6
#define ATHEROS_CONV_VALUE	95

#define ASSOCIATION_REQUEST_SUBTYPE	0
#define ASSOCIATION_RESPONSE_SUBTYPE	1
#define REASSOCIATION_REQUEST_SUBTYPE	2
#define	REASSOCIATION_RESPONSE_SUBTYPE	3
#define	PROBE_REQUEST_SUBTYPE		4
#define	PROBE_RESPONSE_SUBTYPE		5
#define	RESERVED6_SUBTYPE		6
#define	RESERVED7_SUBTYPE		7
#define	BEACON_SUBTYPE			8
#define	ATIM_SUBTYPE			9
#define	DISASSOCIATION_SUBTYPE		10
#define	AUTHENTICATION_SUBTYPE		11
#define	DEAUTHENTICATION_SUBTYPE	12

#define	RESERVED0_SUBTYPE		0
#define	RESERVED9_SUBTYPE		9
#define	POWER_SAVE_SUBTYPE		10
#define	RTS_SUBTYPE			11
#define	CTS_SUBTYPE			12
#define	ACKNOLEDGEMENT_SUBTYPE		13
#define	CF_END_SUBTYPE			14
#define	CF_END_CF_ACK_SUBTYPE		15

#define	DATA_SUBTYPE			0
#define	DATA_CF_ACK_SUBTYPE		1
#define	DATA_CF_POLL_SUBTYPE		2
#define	DATA_CF_ACK_CF_POLL_SUBTYPE	3
#define	NO_DATA_SUBTYPE			4
#define	CF_ACK_SUBTYPE			5
#define	CF_POLL_SUBTYPE			6
#define	CF_ACK_CF_POLL_SUBTYPE		7
#define	RESERVED8_SUBTYPE		8
#define	RESERVED15_SUBTYPE		15

#endif
