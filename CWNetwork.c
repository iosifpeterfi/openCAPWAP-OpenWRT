/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	   *
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

 
#include "CWCommon.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

CWNetworkLev3Service gNetworkPreferredFamily = CW_IPv4;

/*
 * Assume address is valid
 */
__inline__ int CWNetworkGetAddressSize(CWNetworkLev4Address *addrPtr) {
	
	switch ( ((struct sockaddr*)(addrPtr))->sa_family ) {
		
	#ifdef	IPV6
		/* IPv6 is defined in Stevens' library */
		case AF_INET6:
			return sizeof(struct sockaddr_in6);
			break;
	#endif
		case AF_INET:
		default:
			return sizeof(struct sockaddr_in);
	}
}

/* 
 * Send buf on an unconnected UDP socket. Unsafe means that we don't use DTLS.
 */
CWBool CWNetworkSendUnsafeUnconnected(CWSocket sock, 
				      CWNetworkLev4Address *addrPtr,
				      const char *buf,
				      int len) {

	if(buf == NULL || addrPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWUseSockNtop(addrPtr, CWDebugLog(str););

	while(sendto(sock, buf, len, 0, (struct sockaddr*)addrPtr, CWNetworkGetAddressSize(addrPtr)) < 0) {

		if(errno == EINTR) continue;
		CWNetworkRaiseSystemError(CW_ERROR_SENDING);
	}
	return CW_TRUE;
}

/*
 * Send buf on a "connected" UDP socket. Unsafe means that we don't use DTLS.
 */
CWBool CWNetworkSendUnsafeConnected(CWSocket sock, const char *buf, int len) {

	if(buf == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	while(send(sock, buf, len, 0) < 0) {
	
		if(errno == EINTR) continue;
		CWNetworkRaiseSystemError(CW_ERROR_SENDING);
	}
	return CW_TRUE;
}

/* 
 * Receive a datagram on an connected UDP socket (blocking).
 * Unsafe means that we don't use DTLS.
 */
CWBool CWNetworkReceiveUnsafeConnected(CWSocket sock, char *buf, int len, int *readBytesPtr) {
	
	if(buf == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	while((*readBytesPtr = recv(sock, buf, len, 0)) < 0) {

		if(errno == EINTR) continue;
		CWNetworkRaiseSystemError(CW_ERROR_RECEIVING);
	}
	return CW_TRUE;
}

/*
 * Receive a datagram on an unconnected UDP socket (blocking).
 * Unsafe means that we don't use DTLS.
 */
CWBool CWNetworkReceiveUnsafe(CWSocket sock,
			      char *buf,
			      int len,
			      int flags,
			      CWNetworkLev4Address *addrPtr,
			      int *readBytesPtr) {

	socklen_t addrLen = sizeof(CWNetworkLev4Address);
	
	if(buf == NULL || addrPtr == NULL || readBytesPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	while((*readBytesPtr = recvfrom(sock, buf, len, flags, (struct sockaddr*)addrPtr, &addrLen)) < 0) {

		if(errno == EINTR) continue;
		CWNetworkRaiseSystemError(CW_ERROR_RECEIVING);
	}
	return CW_TRUE;
}

/*
 * Init network for client.
 */
CWBool CWNetworkInitSocketClient(CWSocket *sockPtr, CWNetworkLev4Address *addrPtr) {
	
	int yes = 1;
	struct addrinfo hints, *res;
	char myport[8];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	sprintf(myport,"%d",CW_CONTROL_PORT);
	getaddrinfo(NULL, myport, &hints, &res);
	
	/* NULL addrPtr means that we don't want to connect to a 
	 * specific address
	 */
	if(sockPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
#ifdef IPv6
	if(((*sockPtr)=socket((gNetworkPreferredFamily == CW_IPv4) ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
#else
	if(((*sockPtr)=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
#endif
		CWNetworkRaiseSystemError(CW_ERROR_CREATING);
	}

	setsockopt(*sockPtr, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	if(bind(*sockPtr, res->ai_addr, res->ai_addrlen) < 0) {
		close(*sockPtr);
		CWDebugLog("failed to bind Client socket in <%s> line:%d.\n", __func__,__LINE__);
		return CW_FALSE;
	}
	
	CWLog("Binding Client socket with UDP port:%d\n", CW_CONTROL_PORT);

	if(addrPtr != NULL) {
		CWUseSockNtop(((struct sockaddr*)addrPtr), CWDebugLog(str););

		if(connect((*sockPtr), ((struct sockaddr*)addrPtr), CWNetworkGetAddressSize(addrPtr)) < 0) {

			CWNetworkRaiseSystemError(CW_ERROR_CREATING);
		}
	}
	/* allow sending broadcast packets */
	setsockopt(*sockPtr, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
	
	return CW_TRUE;
}

/*
 * Init data channel network for client
 */
CWBool CWNetworkInitSocketClientDataChannel(CWSocket *sockPtr, CWNetworkLev4Address *addrPtr) {
	
	int yes = 1;
	struct addrinfo hints, *res;
	char myport[8];
	CWNetworkLev4Address addrPtrDataChannel;

	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 
	sprintf(myport,"%d",CW_DATA_PORT);
	getaddrinfo(NULL, myport, &hints, &res);

	/* NULL addrPtr means that we don't want to connect to a 
	 * specific address
	 */
	if(sockPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
#ifdef IPv6
	if(((*sockPtr)=socket((gNetworkPreferredFamily == CW_IPv4) ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
#else
	if(((*sockPtr)=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
#endif
		CWNetworkRaiseSystemError(CW_ERROR_CREATING);
	}

	setsockopt(*sockPtr, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

	if(bind(*sockPtr, res->ai_addr, res->ai_addrlen) < 0) {

		close(*sockPtr);
		CWDebugLog("failed to bind Client socket in <%s> line:%d.\n", __func__,__LINE__);
		return CW_FALSE;
	}
	
	CWLog("Binding Client socket with UDP data port:%d\n", CW_DATA_PORT);
	
	if(addrPtr != NULL) {
		CW_COPY_NET_ADDR_PTR(&addrPtrDataChannel,addrPtr);
		sock_set_port_cw((struct sockaddr*)&addrPtrDataChannel, htons(CW_DATA_PORT));
		CWUseSockNtop((struct sockaddr*)&addrPtrDataChannel, CWDebugLog(str););

		if(connect((*sockPtr), (struct sockaddr*)&addrPtrDataChannel, CWNetworkGetAddressSize(&addrPtrDataChannel)) < 0) {

			CWNetworkRaiseSystemError(CW_ERROR_CREATING);
		}
	}

	
	return CW_TRUE;
}


/*
* Wrapper for select
 */
CWBool CWNetworkTimedPollRead(CWSocket sock, struct timeval *timeout) {
	int r;
	
	fd_set fset;
	
	if(timeout == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	FD_ZERO(&fset);
	FD_SET(sock, &fset);

	if((r = select(sock+1, &fset, NULL, NULL, timeout)) == 0) {

		CWDebugLog("Select Time Expired");
		return CWErrorRaise(CW_ERROR_TIME_EXPIRED, NULL);
	} else 
		if (r < 0) {
		
			CWDebugLog("Select Error");
			
			if(errno == EINTR){
				
				CWDebugLog("Select Interrupted by signal");
				return CWErrorRaise(CW_ERROR_INTERRUPTED, NULL);
			}

			CWNetworkRaiseSystemError(CW_ERROR_GENERAL);
		}

	return CW_TRUE;
}

/*
 * Given an host int the form of C string (e.g. "192.168.1.2" or "localhost"),
 * returns the address.
 */
CWBool CWNetworkGetAddressForHost(char *host, CWNetworkLev4Address *addrPtr) {

	struct addrinfo hints, *res, *ressave;
	char serviceName[5];
	CWSocket sock;
	
	if(host == NULL || addrPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_ZERO_MEMORY(&hints, sizeof(struct addrinfo));
	
#ifdef IPv6
	if(gNetworkPreferredFamily == CW_IPv6) {
		hints.ai_family = AF_INET6;
		hints.ai_flags = AI_V4MAPPED;
	} else {
		hints.ai_family = AF_INET;
	}
#else
	hints.ai_family = AF_INET;
#endif
	hints.ai_socktype = SOCK_DGRAM;
	
	/* endianness will be handled by getaddrinfo */
	snprintf(serviceName, 5, "%d", CW_CONTROL_PORT);
	
	if (getaddrinfo(host, serviceName, &hints, &res) !=0 ) {

		return CWErrorRaise(CW_ERROR_GENERAL, "Can't resolve hostname");
	}
	
	ressave = res;
	
	do {
		if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
			/* try next address */
			continue;
		}
		/* success */
		break;
	} while ( (res = res->ai_next) != NULL);
	
	close(sock);
	
	if(res == NULL) { 
		/* error on last iteration */
		CWNetworkRaiseSystemError(CW_ERROR_CREATING);
	}
	
	CW_COPY_NET_ADDR_PTR(addrPtr, (res->ai_addr));
	freeaddrinfo(ressave);
	
	return CW_TRUE;
}
