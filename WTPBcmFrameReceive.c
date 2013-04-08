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


#include "WTPBcmFrameReceive.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif



int extractFrame(CWProtocolMessage** frame, unsigned char* buffer, int len)	//len: frame length 
{
	CW_CREATE_OBJECT_ERR(*frame, CWProtocolMessage, return 0;);
	CWProtocolMessage *auxPtr = *frame;
	CW_CREATE_PROTOCOL_MESSAGE(*auxPtr, len, return 0;);
	memcpy(auxPtr->msg, buffer, len);
	auxPtr->offset=len;
	return 1;
}


int get_mac(u_char *buf)
{
	struct ifreq ifr;
 	int s;


	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return errno;
	}

	
	strncpy(ifr.ifr_name, WL_DEVICE, IFNAMSIZ);
	//ifr.ifr_data = (caddr_t) buf;
	if ((ioctl(s, SIOCGIFHWADDR, &ifr)) < 0) {

		perror(ifr.ifr_name);
		close(s);
		return 0;
	}

	
	/* cleanup */
	close(s);
	memcpy(buf, &(ifr.ifr_hwaddr.sa_data), MAC_ADDR_LEN);
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
	
	return ok;
}




void print_mac(u_char * mac, char * extra) {
  fprint_mac(stdout, mac, extra);
  }

void fprint_mac(FILE * outf, u_char * mac, char * extra) {
  fprintf(outf, "%02X:%02X:%02X:%02X:%02X:%02X%s",
      mac[0] & 0xFF,
      mac[1] & 0xFF,
      mac[2] & 0xFF,
      mac[3] & 0xFF,
      mac[4] & 0xFF,
      mac[5] & 0xFF,
      extra);
  }



CW_THREAD_RETURN_TYPE CWWTPReceiveFrame(void *arg)
{
	pcap_t *handle;
  	char *dev;
  	char errbuf[PCAP_ERRBUF_SIZE];
  	int oldMonitor, newMonitor;
  	struct pcap_pkthdr header;
	u_char wl0_mac_address[MAC_ADDR_LEN];  // interface mac address
  	const u_char *packet;
  	int i;
	printf("2: CWWTPReceiveFrame\n");
	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);
	
	//Get interface mac address
	if(!get_mac(wl0_mac_address)) {

		CWDebugLog("THR FRAME: Failure to get interface mac address");
		EXIT_THREAD
    	}
	
	/*Set interface in a special monitor mode throws WL_GET_MAGIC & WLC_GET_MONITOR*/
	wl_ioctl(WL_DEVICE, WLC_GET_MAGIC, &i, 4);
	if (i != WLC_IOCTL_MAGIC) {

		CWDebugLog("THR FRAME: Wireless magic not correct, not querying wl for info");
		EXIT_THREAD
	}
	else {
	  	wl_ioctl(WL_DEVICE, WLC_GET_MONITOR, &oldMonitor, 4);
	  	newMonitor = 1;
	  	wl_ioctl(WL_DEVICE, WLC_SET_MONITOR, &newMonitor, 4);
	}

	dev = "prism0";
  	handle = pcap_open_live(dev, BUFSIZ, 1, 0, errbuf);

	if (!handle) {
	
		CWDebugLog("THR FRAME: Failure to open pcap!");
		EXIT_THREAD
    	}
	
	CW_REPEAT_FOREVER {
		
		packet = pcap_next(handle, &header);
		if (!packet) break;
		dealWithPacket(&header, packet,wl0_mac_address);
	}

	wl_ioctl(WL_DEVICE, WLC_SET_MONITOR, &oldMonitor, 4);
	pcap_close(handle);		
	
	return(NULL);
}

//////////////////////////////////////////////////////////////////////////////////
void dealWithPacket(struct pcap_pkthdr * header, const u_char * packet, u_char * wl0_mac_address) {
  ieee802_11_hdr * hWifi;
  prism_hdr * hPrism;
  int wfType,packet_len;
  char RSSI = 0, SNR = 0;
  int data_rate = 0;
  prism_did * i;
  char * src = "\0\0\0\0\0\0";
  char * dst = "\0\0\0\0\0\0";
  CWBindingTransportHeaderValues *bindingValuesPtr=NULL;
  CWProtocolMessage *frame=NULL;
  CWBindingDataListElement *listElement=NULL;
  unsigned char *buffer=NULL;

  if (!packet) return;
  if (header->len < sizeof(prism_hdr) + sizeof(ieee802_11_hdr)) return;
  hPrism = (prism_hdr *) packet;
  hWifi = (ieee802_11_hdr *) (packet + (hPrism->msg_length));
  buffer =  (unsigned char *)(packet + (hPrism->msg_length));
  packet_len = hPrism->msg_length;

  if(!macAddrCmp(hWifi->addr1,wl0_mac_address)) return;

  //Parse the prism DIDs
  i = (prism_did *)((char *)hPrism + sizeof(prism_hdr));
  while ((int)i < (int)hWifi) {
    if (i->did == pdn_rssi)  RSSI = *(char *)(i+1);
    if (i->did == pdn_sq) SNR = *(char *)(i+1);
    if (i->did == pdn_datarate) data_rate = (((*(int *)(i+1))/2) * 10);
    i = (prism_did *) ((int)(i+1) + i->length);
    }

  //Establish the frame type
  wfType = ((hWifi->frame_control & 0xF0) >> 4) + ((hWifi->frame_control & 0xC) << 2);
  switch (wfType) {
	case mgt_assocRequest:	src=hWifi->addr2;
      				dst=hWifi->addr1;
				printf("\nAssociation request received from: ");
				print_mac(src,"\n");
				fflush(stdout);
				//CWDebugLog("\nRSSI: %d",RSSI);
				//CWDebugLog("\nSNR: %d",SNR);
				//CWDebugLog("\nData Rate: %d Mbs\n",data_rate);
				CW_CREATE_OBJECT_ERR(bindingValuesPtr, CWBindingTransportHeaderValues, EXIT_THREAD);
				bindingValuesPtr->RSSI=RSSI;
				bindingValuesPtr->SNR=SNR;
				bindingValuesPtr->dataRate=data_rate;	
				CW_CREATE_OBJECT_ERR(listElement, CWBindingDataListElement, EXIT_THREAD);
				if (!extractFrame(&frame, buffer,packet_len - sizeof(prism_hdr)))
						{
							CWDebugLog("THR FRAME: Error extracting a frame");
							EXIT_THREAD
						}
				listElement->frame=frame;
				listElement->bindingValues=bindingValuesPtr;
							
				CWLockSafeList(gFrameList);
				CWAddElementToSafeListTail(gFrameList, listElement, sizeof(CWBindingDataListElement));
				CWUnlockSafeList(gFrameList);		
				break;
  	
	case mgt_assocResponse: break;
  	case mgt_reassocRequest: break;
  	case mgt_reassocResponse: break;
  	case mgt_probeRequest: break;
  	case mgt_probeResponse: break;
  	case mgt_beacon: break;
  	case mgt_disassoc: break;
  	case mgt_auth: break;
  	case mgt_deauth:break;
    	}

  }


