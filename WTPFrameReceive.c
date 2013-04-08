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


#include "WTPFrameReceive.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#define EXIT_FRAME_THREAD(sock)	CWLog("ERROR Handling Frames: application will be closed!");		\
				close(sock);								\
				exit(1);

int getMacAddr(int sock, char* interface, unsigned char* macAddr)
{
	
	
	struct ifreq ethreq;
	int i;

	memset(&ethreq, 0, sizeof(ethreq));
	strncpy(ethreq.ifr_name, interface, IFNAMSIZ);
	if (ioctl(sock, SIOCGIFHWADDR, &ethreq)==-1) 
	{
		return 0;
	}

	for (i=0; i<MAC_ADDR_LEN; i++)	
	{
		macAddr[i]=(unsigned char)ethreq.ifr_hwaddr.sa_data[i];
	}
	CWDebugLog("\n");

	return 1;
}

int extractFrameInfo(char* buffer, char* RSSI, char* SNR, int* dataRate)
{
	int signal, noise;

	*RSSI=buffer[RSSI_BYTE]-ATHEROS_CONV_VALUE;	//RSSI in dBm
	
	signal=buffer[SIGNAL_BYTE]-ATHEROS_CONV_VALUE;	
	noise=buffer[NOISE_BYTE];			
	*SNR=(char)signal-noise;			//RSN in dB

	*dataRate=(buffer[DATARATE_BYTE]/2)*10;		//Data rate in Mbps*10
	return 1;
}

int extractFrame(CWProtocolMessage** frame, unsigned char* buffer, int len)	//len: frame length including prism header
{

	CW_CREATE_OBJECT_ERR(*frame, CWProtocolMessage, return 0;);
	CWProtocolMessage *auxPtr = *frame;
	CW_CREATE_PROTOCOL_MESSAGE(*auxPtr, len-PRISMH_LEN, return 0;);
	memcpy(auxPtr->msg, buffer+PRISMH_LEN, len-PRISMH_LEN);
	auxPtr->offset=len-PRISMH_LEN;
	return 1;
}

int extract802_3_Frame(CWProtocolMessage** frame, unsigned char* buffer, int len)
{
	CW_CREATE_OBJECT_ERR(*frame, CWProtocolMessage, return 0;);
	CWProtocolMessage *auxPtr = *frame;
	CW_CREATE_PROTOCOL_MESSAGE(*auxPtr, len, return 0;);
	memcpy(auxPtr->msg, buffer, len);
	auxPtr->offset=len;
	return 1;
}

int extractAddr(unsigned char* destAddr, unsigned char* sourceAddr, char* frame)
{
	memset(destAddr, 0, MAC_ADDR_LEN);
	memset(sourceAddr, 0, MAC_ADDR_LEN);
	memcpy(destAddr, frame+DEST_ADDR_START, MAC_ADDR_LEN);
	memcpy(sourceAddr, frame+SOURCE_ADDR_START, MAC_ADDR_LEN);

	return 1;
}

int macAddrCmp (unsigned char* addr1, unsigned char* addr2)
{
	int i, ok=1;

/*	CWDebugLog("Address 1:");
	for (i=0; i<MAC_ADDR_LEN; i++)
	{
		CWDebugLog("%02x ", addr1[i]);
	}
	
	CWDebugLog("\nAddress 2:");
	for (i=0; i<MAC_ADDR_LEN; i++)
	{
		CWDebugLog("%02x ", addr2[i]);
	}
	CWDebugLog("\n");*/
	
	for (i=0; i<MAC_ADDR_LEN; i++)
	{
		if (addr1[i]!=addr2[i])
		{ok=0;}
	}

	if (ok==1) {CWDebugLog("MAC Address test: OK\n");}
	else {CWDebugLog("MAC Address test: Failed\n");}
	
	return ok;
}

int gRawSock;
extern int wtpInRunState;

CW_THREAD_RETURN_TYPE CWWTPReceiveFrame(void *arg)
{
	
	const unsigned char VERSION_MASK=3, TYPE_MASK=12, SUBTYPE_MASK=240;
	const unsigned char MANAGEMENT_TYPE=0, CONTROL_TYPE=4, DATA_TYPE=8;
	int n;
	unsigned char buffer[2048];
	unsigned char macAddr[MAC_ADDR_LEN];
	unsigned char destAddr[MAC_ADDR_LEN];
	unsigned char sourceAddr[MAC_ADDR_LEN];
	unsigned char byte0, version=0, type=0, subtype=0;
	struct sockaddr_ll addr;
	CWBindingTransportHeaderValues *bindingValuesPtr=NULL;
	CWProtocolMessage* frame=NULL;
	CWBindingDataListElement* listElement=NULL;
#ifdef PROMODE_ON
	struct ifreq ethreq;
#endif

	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);
	
	if ((gRawSock=socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))<0) 
	{
		CWDebugLog("THR FRAME: Error creating socket");
		CWExitThread();
	}
	
	 /*
         * BUG - UMR02
         * It is better to zero the sockaddr structure before
         * passing it to the kernel.    
         *
         * 19/10/2009 - Donato Capitella
         */
        memset(&addr, 0, sizeof(addr));
	
	addr.sll_family=AF_PACKET;
	addr.sll_protocol=htons(ETH_P_ALL);
	addr.sll_pkttype  = PACKET_HOST;
	addr.sll_ifindex= if_nametoindex(gInterfaceName);
	
	
	if ((bind(gRawSock, (struct sockaddr*)&addr, sizeof(addr)))<0) 
 	{
 		CWDebugLog("THR FRAME: Error binding socket");
 		CWExitThread();
 	}
 	
	if (!getMacAddr(gRawSock, gInterfaceName, macAddr))
 	{
 		CWDebugLog("THR FRAME: Ioctl error");
		EXIT_FRAME_THREAD(gRawSock);
 	}
 	
 #ifdef PROMODE_ON
 	/* Set the network card in promiscuos mode */
 	strncpy(ethreq.ifr_name,gInterfaceName,IFNAMSIZ);
	if (ioctl(gRawSock,SIOCGIFFLAGS,&ethreq)==-1) 
 	{
 		CWDebugLog("THR FRAME: Error ioctl");
		EXIT_FRAME_THREAD(gRawSock);
 	}
 	ethreq.ifr_flags|=IFF_PROMISC;
	if (ioctl(gRawSock,SIOCSIFFLAGS,&ethreq)==-1) 
 	{
 		CWDebugLog("THR FRAME: Error ioctl");
		EXIT_FRAME_THREAD(gRawSock);
 	}
 #endif
 #ifdef FILTER_ON
 	/* Attach the filter to the socket */
	if(setsockopt(gRawSock, SOL_SOCKET, SO_ATTACH_FILTER, &Filter, sizeof(Filter))<0)
 	{
 		CWDebugLog("THR FRAME: Error attaching filter");
		EXIT_FRAME_THREAD(gRawSock);
 	}
 #endif
 	
 	CW_REPEAT_FOREVER 
 	{
		n = recvfrom(gRawSock,buffer,sizeof(buffer),0,NULL,NULL);
		
		if (!wtpInRunState){
			CWLog("WTP is not in RUN state");
			continue;
		}
		
		if (CHECK_PRISM_HEADER)	
		{
			/*CWDebugLog("----------\n");
			CWDebugLog("%d bytes read\n",n);
			CWDebugLog("Prism header OK\n");*/
			if (n>PRISMH_LEN)
			{
				byte0=*(buffer+PRISMH_LEN);
				
				version=byte0 & VERSION_MASK;
				type=byte0 & TYPE_MASK;
				subtype=byte0 & SUBTYPE_MASK;
				subtype>>=4;
				
/*				if(version == (unsigned char) VERSION) {CWDebugLog("Version OK\n");}
				else {CWDebugLog("Wrong Version");}*/
				if(type == (unsigned char) MANAGEMENT_TYPE) 		//Management Frame
				{
					//CWDebugLog("Management Frame\n");
					//CWDebugLog("Subtype: %d ", subtype);
					switch (subtype)
					{
						case ASSOCIATION_REQUEST_SUBTYPE: 
						{
							if (!extractFrame(&frame, buffer, n))
							{
								CWDebugLog("THR FRAME: Error extracting a frame");
								EXIT_FRAME_THREAD(gRawSock);
							}

							extractAddr(destAddr, sourceAddr, frame->msg);
							
							int k;

					
							for(k=0;k<MAC_ADDR_LEN;k++)
							{
								printf("%02x", sourceAddr[k]);	
								if(k!=MAC_ADDR_LEN-1){printf(":");}
							}
							printf("\n");
							fflush(stdout);
							
							CW_CREATE_OBJECT_ERR(bindingValuesPtr, CWBindingTransportHeaderValues, EXIT_FRAME_THREAD(gRawSock););
 							extractFrameInfo((char*)buffer, &(bindingValuesPtr->RSSI),  &(bindingValuesPtr->SNR), &(bindingValuesPtr->dataRate));
							CW_CREATE_OBJECT_ERR(listElement, CWBindingDataListElement, EXIT_FRAME_THREAD(gRawSock););
							listElement->frame=frame;
							listElement->bindingValues=bindingValuesPtr;
							
							//
							CWLockSafeList(gFrameList);
							CWAddElementToSafeListTail(gFrameList, listElement, sizeof(CWBindingDataListElement));
							CWUnlockSafeList(gFrameList);		

							break;
						}
/*						case ASSOCIATION_RESPONSE_SUBTYPE: {CWDebugLog("Association Response\n"); break;}
						case REASSOCIATION_REQUEST_SUBTYPE: {CWDebugLog("Reassociation Request\n"); break;}
						case REASSOCIATION_RESPONSE_SUBTYPE: {CWDebugLog("Reassociation Response\n"); break;}
						case PROBE_REQUEST_SUBTYPE: {CWDebugLog("Probe Request\n"); break;}
						case PROBE_RESPONSE_SUBTYPE: {CWDebugLog("Probe Response\n"); break;}
						case RESERVED6_SUBTYPE: {CWDebugLog("Reserved\n"); break;}
						case RESERVED7_SUBTYPE: {CWDebugLog("Reserved\n"); break;}
						case BEACON_SUBTYPE: {CWDebugLog("Beacon\n"); break;}
						case ATIM_SUBTYPE: {CWDebugLog("ATIM\n"); break;}
						case DISASSOCIATION_SUBTYPE: {CWDebugLog("Disassociation\n"); break;}
						case AUTHENTICATION_SUBTYPE: {CWDebugLog("Authentication\n"); break;}
						case DEAUTHENTICATION_SUBTYPE: {CWDebugLog("Deauthentication\n"); break;}*/
						default: {/*CWDebugLog("Unrecognized Frame\n");*/}
					}
				}
				if(type == (unsigned char) CONTROL_TYPE) 		//Control Frame
				{
/*					CWDebugLog("Control Frame\n");
					//CWDebugLog("Subtype: %d ", subtype);
					switch (subtype)
					{
						case RESERVED0_SUBTYPE: {CWDebugLog("Reserved\n"); break;}
						case RESERVED9_SUBTYPE: {CWDebugLog("Reserved\n"); break;}
						case POWER_SAVE_SUBTYPE: {CWDebugLog("Power Save\n"); break;}
						case RTS_SUBTYPE: {CWDebugLog("RTS\n"); break;}
						case CTS_SUBTYPE: {CWDebugLog("CTS\n"); break;}
						case ACKNOLEDGEMENT_SUBTYPE: {CWDebugLog("Acknoledgement\n"); break;}
						case CF_END_SUBTYPE: {CWDebugLog("CF-End\n"); break;}
						case CF_END_CF_ACK_SUBTYPE: {CWDebugLog("CF-End + CF-Ack\n"); break;}
						default: {CWDebugLog("Unrecognized Frame\n");}
					}*/
				}
				if(type == (unsigned char) DATA_TYPE) 
				{
/*					CWDebugLog("Data Frame\n");
					//CWDebugLog("Subtype: %d ", subtype);
					switch (subtype)
					{
						case DATA_SUBTYPE: {CWDebugLog("Data\n"); break;}
						case DATA_CF_ACK_SUBTYPE: {CWDebugLog("Data + CF-Ack\n"); break;}
						case DATA_CF_POLL_SUBTYPE: {CWDebugLog("Data + CF-Poll\n"); break;}
						case DATA_CF_ACK_CF_POLL_SUBTYPE: {CWDebugLog("Data + CF-Ack + CF-Poll\n"); break;}
						case NO_DATA_SUBTYPE: {CWDebugLog("Null Function (no data)\n"); break;}
						case CF_ACK_SUBTYPE: {CWDebugLog("CF-Ack (no data)\n"); break;}
						case CF_POLL_SUBTYPE: {CWDebugLog("CF-Poll (no data)\n"); break;}
						case CF_ACK_CF_POLL_SUBTYPE: {CWDebugLog("CF-Ack + CF-Poll (no data)\n"); break;}
						case RESERVED8_SUBTYPE: {CWDebugLog("Reserved\n"); break;}
						case RESERVED15_SUBTYPE: {CWDebugLog("Reserved\n"); break;}
						default: {CWDebugLog("Unrecognized Frame\n");}
					}*/
				}
			}
			else CWDebugLog("Malformed Frame");
		}
		else 
		{
			//Assume it is 802.3
			if (n <= (gCWForceMTU-20)){
				if (!extract802_3_Frame(&frame, buffer, n)){
					CWDebugLog("THR FRAME: Error extracting a frame");
					EXIT_FRAME_THREAD(gRawSock);
				}
				CWDebugLog("Send 802.3 data(len:%d) to AC",n);
	;
				CW_CREATE_OBJECT_ERR(listElement, CWBindingDataListElement, EXIT_FRAME_THREAD(gRawSock););
				listElement->frame=frame;
				listElement->bindingValues=NULL;
				
				//
				CWLockSafeList(gFrameList);
				CWAddElementToSafeListTail(gFrameList, listElement, sizeof(CWBindingDataListElement));
				CWUnlockSafeList(gFrameList);
			}else{
				CWDebugLog("size:%d of 802.3 data > MTU:%d",n,gCWForceMTU-20);
 			}
 		}
 	}
 	
	close(gRawSock);
	return(NULL);
}

