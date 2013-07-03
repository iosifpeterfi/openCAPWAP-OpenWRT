/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A         *
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

#ifndef __CAPWAP_CWMultiHomedSocket_HEADER__
#define __CAPWAP_CWMultiHomedSocket_HEADER__

#include "CWNetwork.h"

/*_____________________________________________________*/
/*  *******************___TYPES___*******************  */

typedef struct {
	CWNetworkLev4Address addr;
	CWNetworkLev4Address addrIPv4;
	CWSocket sock;
	enum {
		CW_PRIMARY,
		CW_BROADCAST_OR_ALIAS
	} kind;
	short systemIndex;	// real interface index in the system

	CWNetworkLev4Address dataAddr;
	CWSocket dataSock;
} CWMultiHomedInterface;

typedef struct {
	int count;
	CWMultiHomedInterface *interfaces;
} CWMultiHomedSocket;

/*_____________________________________________________*/
/*  *******************___MACRO___*******************  */

#define CW_COPY_MH_INTERFACE_PTR(int1, int2)        CW_COPY_NET_ADDR_PTR( &((int1)->addr), &((int2)->addr));    \
                            CW_COPY_NET_ADDR_PTR( &((int1)->addrIPv4), &((int2)->addrIPv4));\
                            (int1)->sock = (int2)->sock;                    \
                            (int1)->dataSock = (int2)->dataSock;                \
                            (int1)->kind = (int2)->kind;    \
                            (int1)->systemIndex = (int2)->systemIndex;

/*__________________________________________________________*/
/*  *******************___PROTOTYPES___*******************  */

CWBool CWNetworkInitSocketServerMultiHomed(CWMultiHomedSocket * sockPtr, int port, char **multicastGroups,
					   int multicastGroupsCount);
void CWNetworkCloseMultiHomedSocket(CWMultiHomedSocket * sockPtr);
CWBool CWNetworkUnsafeMultiHomed(CWMultiHomedSocket * sockPtr,
				 void (*CWManageIncomingPacket) (CWSocket, unsigned char *, int, int, CWNetworkLev4Address *,
								 CWBool), CWBool peekRead);
int CWNetworkCountInterfaceAddresses(CWMultiHomedSocket * sockPtr);
CWBool CWNetworkGetInterfaceAddresses(CWMultiHomedSocket * sockPtr, CWNetworkLev4Address ** addressesPtr,
				      struct sockaddr_in **IPv4AddressesPtr);

#endif
