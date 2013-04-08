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

 
#include "CWAC.h"

#include "common.h"
#include "ieee802_11_defs.h"


#define SETBIT(ADDRESS,BIT) (ADDRESS |= (1<<BIT))
#define CLEARBIT(ADDRESS,BIT) (ADDRESS &= ~(1<<BIT))
#define CHECKBIT(ADDRESS,BIT) (ADDRESS & (1<<BIT))

#define TYPE_LEN 2
#define ETH_ALEN 6
#define ETH_HLEN 14

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

__inline__ void CWNetworkDeleteMHInterface(void *intPtr) {

	CW_FREE_OBJECT(intPtr);
}

int from_8023_to_80211( unsigned char *inbuffer,int inlen, unsigned char *outbuffer, unsigned char *own_addr){

	int indx=0;
	struct ieee80211_hdr hdr;
	os_memset(&hdr,0,sizeof(struct ieee80211_hdr));

	hdr.frame_control = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
	hdr.duration_id = 0;
	hdr.seq_ctrl = 0;

	os_memcpy(hdr.addr1, inbuffer, ETH_ALEN);
	os_memcpy(hdr.addr2, own_addr, ETH_ALEN);
	os_memcpy(hdr.addr3, inbuffer + ETH_ALEN, ETH_ALEN);
	CLEARBIT(hdr.frame_control,8);
	SETBIT(hdr.frame_control,9);	
	
	os_memcpy(outbuffer + indx,&hdr,sizeof(hdr));
	indx += sizeof(hdr);
	os_memcpy(outbuffer + indx, inbuffer, inlen);
	indx += inlen;
	
	return indx;
}



/*
 * Multihomed sockets maps the system index for each interface to a array-like
 * int index in range 0-(# of interfaces -1). This function returns the int
 * index given the system index of an interface managed by the given multihomed
 * socket.
 */
int CWNetworkGetInterfaceIndexFromSystemIndex(CWMultiHomedSocket *sockPtr,
					      int systemIndex) {
	
	int i, c;
	
	if(sockPtr == NULL || systemIndex == -1) return -1;

	for(i = 0, c = 0; i < sockPtr->count; i++) {

		if(sockPtr->interfaces[i].kind == CW_PRIMARY) {

			/* each primary interface increments the int index */
			if(sockPtr->interfaces[i].systemIndex == systemIndex) 
				return c;
			c++;
		}
	}
	return -1;
}

/*
 * Check if the interface with system index systemIndex is already managed by
 * the multihomed socket. If the answer is yes, returns informations on that 
 * interface, returns NULL otherwise.
 */
CWMultiHomedInterface *CWNetworkGetInterfaceAlreadyStored(CWList list,
							  short systemIndex) {

	CWListElement *el;
	
	for(el = list; el != NULL; el = el->next) {

		if(((CWMultiHomedInterface*)(el->data))->systemIndex == systemIndex &&
		   ((CWMultiHomedInterface*)(el->data))->kind == CW_PRIMARY) 
			
			return (CWMultiHomedInterface*) el->data;
	}
	return NULL;
}

/*
 * Init multihomed socket. Will bind a socket for each interface + each 
 * broadcast address + the wildcard addres + each multicast address in 
 * multicastGroups.
 */
CWBool CWNetworkInitSocketServerMultiHomed(CWMultiHomedSocket *sockPtr, 
					   int port, 
					   char **multicastGroups, 
					   int multicastGroupsCount) {

	struct ifi_info	*ifi, *ifihead;
	CWNetworkLev4Address wildaddr;
    	int yes = 1;
	CWSocket sock;
	CWMultiHomedInterface *p;
	CWList interfaceList = CW_LIST_INIT;
	CWListElement *el = NULL;
	int i;
	
	if(sockPtr == NULL) return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	sockPtr->count = 0;
	
	/* 
	 * note: if get_ifi_info is called with AF_INET6 on an host that doesn't
	 * support IPv6, it'll simply act like if it was called with AF_INET.
	 * Consider aliases as different interfaces (last arg of get_ifi_info is 1).
	 * Why? Just to increase the funny side of the thing.
	 */
#ifdef CW_DEBUGGING
	/* for each network interface... */
	for (ifihead = ifi = get_ifi_info((gNetworkPreferredFamily == CW_IPv6) ? AF_INET6 : AF_INET, 1); ifi != NULL; ifi = ifi->ifi_next) { 
#else
	/* for each network interface... */
	for (ifihead = ifi = get_ifi_info((gNetworkPreferredFamily == CW_IPv6) ? AF_INET6 : AF_INET, 0); ifi != NULL; ifi = ifi->ifi_next) {
#endif
		/* bind a unicast address */
		if((sock = socket(ifi->ifi_addr->sa_family, SOCK_DGRAM, 0)) < 0) {

			free_ifi_info(ifihead);
			CWNetworkRaiseSystemError(CW_ERROR_CREATING);
		}
		
		/* reuse address */
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		
		/* bind address */
		sock_set_port_cw(ifi->ifi_addr, htons(port));
		
		if(bind(sock, (struct sockaddr*) ifi->ifi_addr, CWNetworkGetAddressSize((CWNetworkLev4Address*)ifi->ifi_addr)) < 0) {

			close(sock);
			CWUseSockNtop(ifi->ifi_addr, CWDebugLog("failed %s", str););
			continue;
			/* CWNetworkRaiseSystemError(CW_ERROR_CREATING); */
		}
		
		CWUseSockNtop(ifi->ifi_addr, 
			      CWLog("bound %s (%d, %s)", str, ifi->ifi_index, ifi->ifi_name););
		
		/* store socket inside multihomed socket */
		CW_CREATE_OBJECT_ERR(p, CWMultiHomedInterface, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		p->sock = sock;
		if(CWNetworkGetInterfaceAlreadyStored(interfaceList, ifi->ifi_index) == NULL &&
		   strncmp(ifi->ifi_name, "lo", 2)) { /* don't consider loopback an interface
							 (even if we accept packets from loopback) */
			CWDebugLog("Primary Address");
			p->kind = CW_PRIMARY;

		} else {
			/* should be BROADCAST_OR_ALIAS_OR_MULTICAST_OR_LOOPBACK ;-) */
			p->kind = CW_BROADCAST_OR_ALIAS;
#ifdef CW_DEBUGGING
#if 0
				if(!strncmp(ifi->ifi_name, "lo", 2)) {
					p->kind = CW_PRIMARY;
				}
#endif
#endif
		}

		p->systemIndex = ifi->ifi_index;
		
		/* the next field is useful only if we are an IPv6 server. In
		 * this case, p->addr contains the IPv6 address of the interface 
		 * and p->addrIPv4 contains the equivalent IPv4 address. On the
		 * other side, if we are an IPv4 server p->addr contains the 
		 * IPv4 address of the interface and p->addrIPv4 is garbage.
		 */
		p->addrIPv4.ss_family = AF_UNSPEC;

		CW_COPY_NET_ADDR_PTR(&(p->addr), ifi->ifi_addr);
																	// Todd: Bind data channel to port 5427
		/* bind a unicast address of data UDP stream */
		if((sock = socket(ifi->ifi_addr->sa_family, SOCK_DGRAM, 0)) < 0) {
			free_ifi_info(ifihead);
			CWNetworkRaiseSystemError(CW_ERROR_CREATING);
		}
		
		/* reuse address */
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		
		/* bind address */
		sock_set_port_cw(ifi->ifi_addr, htons(port+1));
		
		if(bind(sock, (struct sockaddr*) ifi->ifi_addr, CWNetworkGetAddressSize((CWNetworkLev4Address*)ifi->ifi_addr)) < 0) {
			close(sock);
			CWUseSockNtop(ifi->ifi_addr, CWDebugLog("failed %s", str););
			continue;
			/* CWNetworkRaiseSystemError(CW_ERROR_CREATING); */
		}
		
		CWUseSockNtop(ifi->ifi_addr, 
			      CWLog("Data channel bound %s (%d, %s)", str, ifi->ifi_index, ifi->ifi_name););
			      
		CW_COPY_NET_ADDR_PTR(&(p->dataAddr), ifi->ifi_addr);
		p->dataSock = sock;

		if(!CWAddElementToList(&interfaceList, p)) {
		
			return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
		}
		/* we add a socket to the multihomed socket */
		sockPtr->count++;	
		
		if (ifi->ifi_flags & IFF_BROADCAST) { 
			/* try to bind broadcast address */
			if((sock = socket(ifi->ifi_addr->sa_family, SOCK_DGRAM, 0)) < 0) {

				free_ifi_info(ifihead);
				CWDeleteList(&interfaceList, CWNetworkDeleteMHInterface);
				CWNetworkRaiseSystemError(CW_ERROR_CREATING);
			}
			
			/* reuse address */
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
			
			sock_set_port_cw(ifi->ifi_brdaddr, htons(port));
			
			if (bind(sock, (struct sockaddr*)ifi->ifi_brdaddr, 
				 CWNetworkGetAddressSize((CWNetworkLev4Address*)ifi->ifi_brdaddr)) < 0) {

				close(sock);
				if (errno == EADDRINUSE) {
					CWUseSockNtop(ifi->ifi_brdaddr,
						CWDebugLog("EADDRINUSE: %s", str);
					);
					continue;
				} else {
					CWUseSockNtop(ifi->ifi_brdaddr,
						CWDebugLog("failed %s", str);
					);
					continue;
					/* CWDeleteList(&interfaceList, CWNetworkDeleteMHInterface); */
					/* CWNetworkRaiseSystemError(CW_ERROR_CREATING); */
				}
			}
			
			CWUseSockNtop(ifi->ifi_brdaddr,
				      CWLog("bound %s (%d, %s)", 
				      str,
				      ifi->ifi_index,
				      ifi->ifi_name););
			
			/* store socket inside multihomed socket */
			
			CW_CREATE_OBJECT_ERR(p, CWMultiHomedInterface, 
					     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
			p->sock = sock;
			p->kind = CW_BROADCAST_OR_ALIAS;
			p->systemIndex = ifi->ifi_index;
			CW_COPY_NET_ADDR_PTR(&(p->addr), ifi->ifi_brdaddr);
			
			/* The next field is useful only if we are an IPv6 server.
			 * In this case, p->addr contains the IPv6 address of the 
			 * interface and p->addrIPv4 contains the equivalent IPv4 
			 * address. On the other side, if we are an IPv4 server 
			 * p->addr contains the IPv4 address of the interface and
			 * p->addrIPv4 is garbage.
			 */
			p->addrIPv4.ss_family = AF_UNSPEC;

			if(!CWAddElementToList(&interfaceList, p)) {

				return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
			}
			/* we add a socket to the multihomed socket */
			sockPtr->count++;
		}
	}

	/* get_ifi_info returned an error */
	if(ifihead == NULL) {

		CWDeleteList(&interfaceList, CWNetworkDeleteMHInterface);
		return CWErrorRaise(CW_ERROR_NEED_RESOURCE, 
				    "Error With get_ifi_info()");
	}
	free_ifi_info(ifihead);
	
#ifdef IPV6
	/* we are an IPv6 server */
	if(gNetworkPreferredFamily == CW_IPv6) {
		/* 
		 * Store IPv4 addresses for our interfaces in the field "addrIPv4".
		 * Consider aliases as different interfaces (last arg of get_ifi_info is 1).
		 * Why? Just to increase the funny side of the thing.
		 */
#ifdef CW_DEBUGGING
		for (ifihead = ifi = get_ifi_info(AF_INET, 1); ifi != NULL; ifi = ifi->ifi_next) {
#else
		for (ifihead = ifi = get_ifi_info(AF_INET, 0); ifi != NULL; ifi = ifi->ifi_next) {
#endif
			CWMultiHomedInterface *s = CWNetworkGetInterfaceAlreadyStored(interfaceList, ifi->ifi_index);
			
			if(s == NULL ||
			   s->kind != CW_PRIMARY ||
			   s->addrIPv4.ss_family != AF_UNSPEC ||
			   ifi->ifi_addr->sa_family != AF_INET) continue;
			
			CW_COPY_NET_ADDR_PTR(&(s->addrIPv4), ifi->ifi_addr);
			
			CWUseSockNtop(&(s->addrIPv4),
				CWDebugLog("IPv4 address %s (%d, %s)", str, ifi->ifi_index, ifi->ifi_name););
		}
		/* get_ifi_info returned an error */
		if(ifihead == NULL) {

			CWDeleteList(&interfaceList, CWNetworkDeleteMHInterface);
			return CWErrorRaise(CW_ERROR_NEED_RESOURCE, 
					    "Error with get_ifi_info()");
		}
		free_ifi_info(ifihead);
	}
#endif
	/* bind wildcard address */
#ifdef	IPV6
	if (gNetworkPreferredFamily == CW_IPv6) {

		if((sock = socket(AF_INET6,SOCK_DGRAM,0)) < 0) {
			goto fail;
		}
	} else
#endif
	{
		if((sock = socket(AF_INET,SOCK_DGRAM, 0)) < 0) goto fail;
	}
	
	goto success;
	
fail:
	CWDeleteList(&interfaceList, CWNetworkDeleteMHInterface);
	CWNetworkRaiseSystemError(CW_ERROR_CREATING); /* this wil return */
	/* not reached */
	
success:
	/* reuse address */
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	CW_ZERO_MEMORY(&wildaddr, sizeof(wildaddr));
	
#ifdef	IPV6
	if (gNetworkPreferredFamily == CW_IPv6) {
		/* fill wildaddr considering it an IPv6 addr */
		struct sockaddr_in6 *a = (struct sockaddr_in6 *) &wildaddr;
		a->sin6_family = AF_INET6;
		a->sin6_addr = in6addr_any;
		a->sin6_port = htons(port);
	} else
#endif
	{
		/* fill wildaddr considering it an IPv4 addr */
		struct sockaddr_in *a = (struct sockaddr_in *) &wildaddr;
		a->sin_family = AF_INET;
		a->sin_addr.s_addr = htonl(INADDR_ANY);
		a->sin_port = htons(port);
	}
	
	if(bind(sock, (struct sockaddr*) &wildaddr, CWNetworkGetAddressSize(&wildaddr)) < 0) {
		close(sock);
		CWDeleteList(&interfaceList, CWNetworkDeleteMHInterface);
		CWNetworkRaiseSystemError(CW_ERROR_CREATING);
	}
	
	CWUseSockNtop(&wildaddr,
		CWLog("bound %s", str);
	);
	
	CW_CREATE_OBJECT_ERR(p, CWMultiHomedInterface, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	p->sock = sock;
	p->kind = CW_BROADCAST_OR_ALIAS;
	p->systemIndex = -1; /* make sure this can't be 
				confused with an interface */
	
	/* addrIPv4 field for the wildcard address cause it 
	 * is garbage in both cases (IPv4 + IPv6)
	 */
	p->addrIPv4.ss_family = AF_UNSPEC;

	CW_COPY_NET_ADDR_PTR(&(p->addr), &wildaddr);
	if(!CWAddElementToList(&interfaceList, p)) {
		return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
	}
	sockPtr->count++;

	/* bind multicast addresses */
	for(i = 0; i < multicastGroupsCount; i++) {
		struct addrinfo hints, *res, *ressave;
		char serviceName[5];
		CWSocket sock;
		
		CW_ZERO_MEMORY(&hints, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_DGRAM;
		
		/* endianness will be handled by getaddrinfo */
		snprintf(serviceName, 5, "%d", CW_CONTROL_PORT);
		
		CWLog("Joining Multicast Group: %s...", multicastGroups[i]);
		
		if (getaddrinfo(multicastGroups[i], serviceName, &hints, &res) != 0 ) {

			CWNetworkRaiseSystemError(CW_ERROR_CREATING);
		}
		ressave = res;

		do {
			if((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
				continue; /* try next address */
			}
			
			/* reuse address */
			setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	
			if(bind(sock, res->ai_addr, res->ai_addrlen) == 0) break; /* success */
			
			close(sock); /* failure */
		} while ( (res = res->ai_next) != NULL);
		
		if(res == NULL) { /* error on last iteration */
			CWNetworkRaiseSystemError(CW_ERROR_CREATING);
		}
		
		if(mcast_join(sock, res->ai_addr, res->ai_addrlen, NULL, 0) != 0) {
			CWNetworkRaiseSystemError(CW_ERROR_CREATING);
		}
		
		CWUseSockNtop((res->ai_addr),
			CWLog("Joined Multicast Group: %s", str);
		);
		
		CW_CREATE_OBJECT_ERR(p, CWMultiHomedInterface, 
				     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		p->sock = sock;
		p->kind = CW_BROADCAST_OR_ALIAS;
		p->systemIndex = -1;
		
		
		p->addrIPv4.ss_family = AF_UNSPEC;
		
		CW_COPY_NET_ADDR_PTR(&(p->addr), res->ai_addr);
		if(!CWAddElementToList(&interfaceList, p)) {
			return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
		}
		sockPtr->count++; /* we add a socket to the multihomed socket */
		
		freeaddrinfo(ressave);
	}
	
	
	/*
	 * Lists are fun when you don't know how many sockets will not give an
	 * error on creating/binding, but now that we know the exact number we
	 * convert it into an array. The "interfaces" field of CWMultiHomedSocket
	 * is actually an array.
	 */
	CW_CREATE_ARRAY_ERR((sockPtr->interfaces), sockPtr->count, CWMultiHomedInterface,
					return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	/* create array from list */
	for(el = interfaceList, i = 0; el != NULL; el = el->next, i++) {
		CW_COPY_MH_INTERFACE_PTR(&((sockPtr->interfaces)[i]), ((CWMultiHomedInterface*)(el->data)));
	}
	
	/* delete the list */
	CWDeleteList(&interfaceList, CWNetworkDeleteMHInterface);
	
	return CW_TRUE;
}

void CWNetworkCloseMultiHomedSocket(CWMultiHomedSocket *sockPtr) {

	int i = 0;
	
	if(sockPtr == NULL || sockPtr->interfaces == NULL) 
		return;
	
	for(i = 0; i < sockPtr->count; i++) 
		close(sockPtr->interfaces[i].sock);

	CW_FREE_OBJECT(sockPtr->interfaces);
	sockPtr->count = 0;
}

int get_mac_addr( unsigned char *outBuf,char *eth_name){
	
	struct ifreq s;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(s.ifr_name, eth_name);
	if(!ioctl(fd, SIOCGIFHWADDR, &s))
		memcpy(outBuf, s.ifr_addr.sa_data, 6);
	
	return 0;
}


/*
 * Blocks until one ore more interfaces are ready to read something. When there
 * is at least one packet pending, call CWManageIncomingPacket() for each pending
 * packet, then return.
 */
CWBool CWNetworkUnsafeMultiHomed(CWMultiHomedSocket *sockPtr, 
				 void (*CWManageIncomingPacket)(CWSocket, 
					 			char *, 
								int, 
								int, 
								CWNetworkLev4Address*,
								CWBool),
				 CWBool peekRead) {
	fd_set fset;
	int max = 0, i;
	CWNetworkLev4Address addr;
	CWNetworkLev4Address address;
 	
	int k;
	int fragmentsNum = 0;
	CWProtocolMessage *completeMsgPtr = NULL;
	CWProtocolMessage* frame=NULL;
	int dataSocket=0;
	int readBytes;
	int flags = ((peekRead != CW_FALSE) ? MSG_PEEK : 0);	
	char buf[CW_BUFFER_SIZE];
	
	if (sockPtr == NULL || sockPtr->count == 0 || CWManageIncomingPacket == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	FD_ZERO(&fset);

	/* select() on all the sockets */
	for(i = 0; i < sockPtr->count; i++) {
	
		FD_SET(sockPtr->interfaces[i].sock, &fset);

		if (sockPtr->interfaces[i].sock > max)
			max = sockPtr->interfaces[i].sock;

		if (sockPtr->interfaces[i].dataSock != 0){					// Todd: add tap device to 'select '
		      FD_SET(sockPtr->interfaces[i].dataSock, &fset);

		      if (sockPtr->interfaces[i].dataSock > max)
			      max = sockPtr->interfaces[i].dataSock;
		}
	}

	for(i = 0; i < gMaxWTPs; i++) {
		if (gWTPs[i].tap_fd!= 0){
		      FD_SET(gWTPs[i].tap_fd, &fset);

		      if (gWTPs[i].tap_fd > max)
			      max = gWTPs[i].tap_fd;
		}
 	}

	while(select(max+1, &fset, NULL, NULL, NULL) < 0) {
		
		if (errno != EINTR) {
			CWNetworkRaiseSystemError(CW_ERROR_GENERAL);
		}
	}
	
	/* calls CWManageIncomingPacket() for each interface 
	 * that has an incoming packet 
	 */
	
	for(i = 0; i < gMaxWTPs; i++) {
  
		if (FD_ISSET(gWTPs[i].tap_fd, &fset)) {
			
			readBytes = read(gWTPs[i].tap_fd,buf,CW_BUFFER_SIZE);							//Todd: read from TAP then forward to WTP through data channel
			CWDebugLog("gWTPs[%d].tap_fd:%d is set,data(%d bytes)",i,gWTPs[i].tap_fd, readBytes);
			
			if(readBytes < 0) {
				CWDebugLog("Reading from tap interface");
				perror("Reading from interface");
				close(gWTPs[i].tap_fd);
				gWTPs[i].tap_fd=0;
			}
			
			if (gWTPs[i].currentState != CW_ENTER_RUN){
			      CWDebugLog("WTP %d is not in RUN State. The packet was dropped.",i);
			      continue;
			}else{
				
				unsigned char macAddrTap[6];
				get_mac_addr(macAddrTap, gWTPs[i].tap_name);
				unsigned char buf80211[CW_BUFFER_SIZE + 24];
				int readByest80211 = from_8023_to_80211(buf, readBytes, buf80211, macAddrTap);
				CW_CREATE_OBJECT_ERR(frame, CWProtocolMessage, return 0;);
				CW_CREATE_PROTOCOL_MESSAGE(*frame, readByest80211, return 0;);
				memcpy(frame->msg, buf80211, readByest80211);
				frame->offset = readByest80211;
				frame->data_msgType = CW_IEEE_802_11_FRAME_TYPE;

				if (!CWAssembleDataMessage(&completeMsgPtr, 
							  &fragmentsNum, 
							  gWTPs[i].pathMTU, 
							  frame, 
							  NULL,
							  CW_PACKET_PLAIN
							  ,0))
				{
					
					
					for(k = 0; k < fragmentsNum; k++)
					{
						CW_FREE_PROTOCOL_MESSAGE(completeMsgPtr[k]);
					}
					CW_FREE_OBJECT(completeMsgPtr);
					CW_FREE_PROTOCOL_MESSAGE(*frame);
					CW_FREE_OBJECT(frame);
					continue;
				}
				
				for(k = 0; k < sockPtr->count; k++) {
				      if (sockPtr->interfaces[k].sock == gWTPs[i].socket){
					  dataSocket = sockPtr->interfaces[k].dataSock;
					  CW_COPY_NET_ADDR_PTR(&address,&(gWTPs[i].address));
					  break;
				      }
				}
		
				
				if (dataSocket == 0){
				      CWDebugLog("data socket of WTP %d isn't ready.");
				      continue;
				}
				
				/* Set port and address of data tunnel */
				sock_set_port_cw((struct sockaddr *)&(address), htons(CW_DATA_PORT));

				for (k = 0; k < fragmentsNum; k++) 
				{
					if(!CWNetworkSendUnsafeUnconnected(	dataSocket, 
										&(address), 
										completeMsgPtr[k].msg, 
										completeMsgPtr[k].offset)	) {
						CWDebugLog("Failure sending Request");
						break;
					}
				}
				for (k = 0; k < fragmentsNum; k++)
				{
					CW_FREE_PROTOCOL_MESSAGE(completeMsgPtr[k]);
				}
				
				CW_FREE_OBJECT(completeMsgPtr);				
				CW_FREE_PROTOCOL_MESSAGE(*(frame));
				CW_FREE_OBJECT(frame);
				
			}
		}
	}

	for(i = 0; i < sockPtr->count; i++) {

		if(FD_ISSET(sockPtr->interfaces[i].sock, &fset)) {
			int readBytes;

			/*	
			CWUseSockNtop(&(sockPtr->interfaces[i].addr),
				CWDebugLog("Ready on %s", str);
			);
			*/
			
			CW_ZERO_MEMORY(buf, CW_BUFFER_SIZE);
			
			/* message */
			if(!CWErr(CWNetworkReceiveUnsafe(sockPtr->interfaces[i].sock, buf, CW_BUFFER_SIZE-1, flags, &addr, &readBytes))) {

				sleep(1);
				continue;
			}
			
			CWManageIncomingPacket(sockPtr->interfaces[i].sock, 
					       buf, 
					       readBytes,
					       CWNetworkGetInterfaceIndexFromSystemIndex(sockPtr, sockPtr->interfaces[i].systemIndex),
					       &addr,CW_FALSE);
		}
	  
	  
		if(FD_ISSET(sockPtr->interfaces[i].dataSock, &fset)) {						//Todd: Bridge 802.3 packets of WTPs into AC
			int readBytes;

	
			CW_ZERO_MEMORY(buf, CW_BUFFER_SIZE);
			
			/* message */
			if(!CWErr(CWNetworkReceiveUnsafe(sockPtr->interfaces[i].dataSock, buf, CW_BUFFER_SIZE-1, flags, &addr, &readBytes))) {

				sleep(1);
				continue;
			}

			CWManageIncomingPacket(sockPtr->interfaces[i].dataSock, 
					       buf, 
					       readBytes,
					       CWNetworkGetInterfaceIndexFromSystemIndex(sockPtr, sockPtr->interfaces[i].systemIndex),
					       &addr,CW_TRUE);
		}
		/* else {CWDebugLog("~~~~~~~Non Ready on....~~~~~~");} */
	}
	return CW_TRUE;
}


/* count distinct interfaces managed by the multihomed socket */
int CWNetworkCountInterfaceAddresses(CWMultiHomedSocket *sockPtr) {

	int count = 0;
	int i;
	
	if(sockPtr == NULL) return 0;
	
	for(i = 0; i < sockPtr->count; i++) {
	
		if(sockPtr->interfaces[i].kind == CW_PRIMARY) count++;
	}

	return count;
}

/* 
 * Get the addresses of each distinct interface managed by the multihomed 
 * socket. If we are an IPv6 server element with index i of addressesPtr contains
 * the IPv6 address of the interface at index i (our mapped index, not system 
 * index) and the element at index i of IPv4AddressesPtr contains the IPv4 
 * equivalent address for the interface at index i. If we are an IPv4 server,
 * addressesPtr are the IPv4 addresses and IPv4AddressesPtr is garbage.
 */
CWBool CWNetworkGetInterfaceAddresses(CWMultiHomedSocket *sockPtr,
				      CWNetworkLev4Address **addressesPtr,
				      struct sockaddr_in **IPv4AddressesPtr) {
	int i, j;
	
	if(sockPtr == NULL || addressesPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CW_CREATE_ARRAY_ERR(*addressesPtr, 
			    CWNetworkCountInterfaceAddresses(sockPtr), 
			    CWNetworkLev4Address,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
	if(IPv4AddressesPtr != NULL && gNetworkPreferredFamily == CW_IPv6) {

		CW_CREATE_ARRAY_ERR(*IPv4AddressesPtr, 
				    CWNetworkCountInterfaceAddresses(sockPtr),
				    struct sockaddr_in,
				    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	}
	
	for(i = 0, j = 0; i < sockPtr->count; i++) {

		if(sockPtr->interfaces[i].kind == CW_PRIMARY) {

			CW_COPY_NET_ADDR_PTR(&((*addressesPtr)[j]), ((CWNetworkLev4Address*)&(sockPtr->interfaces[i].addr)));

			if(IPv4AddressesPtr != NULL && gNetworkPreferredFamily == CW_IPv6) {
				
				CW_COPY_NET_ADDR_PTR(&((*IPv4AddressesPtr)[j]), ((CWNetworkLev4Address*)&(sockPtr->interfaces[i].addrIPv4)));
			}
			j++;
		}
	}
	return CW_TRUE;
}
