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

#include "WTPmacFrameReceive.h"
#include "common.h"
#include "ieee802_11_defs.h"

#define TYPE_LEN 2
#define ETH_ALEN 6
#define ETH_HLEN 14
#define FRAME_80211_LEN 24

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#define EXIT_FRAME_THREAD(sock) CWLog("ERROR Handling Frames: application will be closed!");        \
                close(sock);                                \
                exit(1);

int getMacAddr(int sock, char *interface, unsigned char *macAddr)
{

	struct ifreq s;
	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(s.ifr_name, interface);
	if (!ioctl(fd, SIOCGIFHWADDR, &s))
		memcpy(macAddr, s.ifr_addr.sa_data, MAC_ADDR_LEN);

	CWDebugLog("\n");

	return 1;
}

int extractFrameInfo(char *buffer, char *RSSI, char *SNR, int *dataRate)
{
	int signal, noise;

	*RSSI = buffer[RSSI_BYTE] - ATHEROS_CONV_VALUE;	//RSSI in dBm

	signal = buffer[SIGNAL_BYTE] - ATHEROS_CONV_VALUE;
	noise = buffer[NOISE_BYTE];
	*SNR = (char)signal - noise;	//RSN in dB

	*dataRate = (buffer[DATARATE_BYTE] / 2) * 10;	//Data rate in Mbps*10
	return 1;
}

int extractFrame(CWProtocolMessage ** frame, unsigned char *buffer, int len)
{

	CW_CREATE_OBJECT_ERR(*frame, CWProtocolMessage, return 0;
	    );
	CWProtocolMessage *auxPtr = *frame;
	CW_CREATE_PROTOCOL_MESSAGE(*auxPtr, len - PRISMH_LEN, return 0;
	    );
	memcpy(auxPtr->msg, buffer + PRISMH_LEN, len - PRISMH_LEN);
	auxPtr->offset = len - PRISMH_LEN;
	return 1;
}

int extract802_11_Frame(CWProtocolMessage ** frame, unsigned char *buffer, int len)
{
	CW_CREATE_OBJECT_ERR(*frame, CWProtocolMessage, return 0;
	    );
	CWProtocolMessage *auxPtr = *frame;
	CW_CREATE_PROTOCOL_MESSAGE(*auxPtr, len, return 0;
	    );
	memcpy(auxPtr->msg, buffer, len);
	auxPtr->offset = len;
	return 1;
}

int extractAddr(unsigned char *destAddr, unsigned char *sourceAddr, char *frame)
{
	memset(destAddr, 0, MAC_ADDR_LEN);
	memset(sourceAddr, 0, MAC_ADDR_LEN);
	memcpy(destAddr, frame + DEST_ADDR_START, MAC_ADDR_LEN);
	memcpy(sourceAddr, frame + SOURCE_ADDR_START, MAC_ADDR_LEN);

	return 1;
}

int macAddrCmp(unsigned char *addr1, unsigned char *addr2)
{
	int i, ok = 1;

	for (i = 0; i < MAC_ADDR_LEN; i++) {
		if (addr1[i] != addr2[i]) {
			ok = 0;
		}
	}

	if (ok == 1) {
		CWDebugLog("MAC Address test: OK\n");
	} else {
		CWDebugLog("MAC Address test: Failed\n");
	}

	return ok;
}

int from_8023_to_80211(unsigned char *inbuffer, int inlen, unsigned char *outbuffer, unsigned char *own_addr)
{

	int indx = 0;
	struct ieee80211_hdr hdr;
	os_memset(&hdr, 0, sizeof(struct ieee80211_hdr));

	hdr.frame_control = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA) | host_to_le16(WLAN_FC_TODS);
	hdr.duration_id = 0;
	hdr.seq_ctrl = 0;

	os_memcpy(hdr.addr1, own_addr, ETH_ALEN);
	os_memcpy(hdr.addr2, inbuffer + ETH_ALEN, ETH_ALEN);
	os_memcpy(hdr.addr3, inbuffer, ETH_ALEN);

	os_memcpy(outbuffer + indx, &hdr, sizeof(hdr));
	indx += sizeof(hdr);
	os_memcpy(outbuffer + indx, inbuffer, inlen);
	indx += inlen;

	return indx;
}

int gRawSock;
extern int wtpInRunState;

int CWWTPSendFrame(unsigned char *buf, int len)
{

	if (send(gRawSock, buf + FRAME_80211_LEN, len - FRAME_80211_LEN, 0) < 1) {
		CWDebugLog("Error to send frame on raw socket");
		return -1;
	}
	CWDebugLog("Send (%d) bytes on raw socket", len - FRAME_80211_LEN);

	return 1;

}

CW_THREAD_RETURN_TYPE CWWTPReceiveFrame(void *arg)
{

	int n, encaps_len;
	unsigned char buffer[CW_BUFFER_SIZE];
	unsigned char buf80211[CW_BUFFER_SIZE];
	unsigned char macAddr[MAC_ADDR_LEN];

	struct sockaddr_ll addr;

	CWBindingDataListElement *listElement = NULL;
	struct ifreq ethreq;

	CWThreadSetSignals(SIG_BLOCK, 1, SIGALRM);

	if ((gRawSock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		CWDebugLog("THR FRAME: Error creating socket");
		CWExitThread();
	}
	memset(&addr, 0, sizeof(addr));

	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETH_P_ALL);
	addr.sll_pkttype = PACKET_HOST;
	addr.sll_ifindex = if_nametoindex(gRadioInterfaceName_0);

	if ((bind(gRawSock, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
		CWDebugLog("THR FRAME: Error binding socket");
		CWExitThread();
	}

	if (!getMacAddr(gRawSock, gRadioInterfaceName_0, macAddr)) {
		CWDebugLog("THR FRAME: Ioctl error");
		EXIT_FRAME_THREAD(gRawSock);
	}

	/* Set the network card in promiscuos mode */
	strncpy(ethreq.ifr_name, gRadioInterfaceName_0, IFNAMSIZ);
	if (ioctl(gRawSock, SIOCGIFFLAGS, &ethreq) == -1) {
		CWDebugLog("THR FRAME: Error ioctl");
		EXIT_FRAME_THREAD(gRawSock);
	}
	ethreq.ifr_flags |= IFF_PROMISC;
	if (ioctl(gRawSock, SIOCSIFFLAGS, &ethreq) == -1) {
		CWDebugLog("THR FRAME: Error ioctl");
		EXIT_FRAME_THREAD(gRawSock);
	}

#ifdef FILTER_ON
	/* Attach the filter to the socket */
	if (setsockopt(gRawSock, SOL_SOCKET, SO_ATTACH_FILTER, &Filter, sizeof(Filter)) < 0) {
		CWDebugLog("THR FRAME: Error attaching filter");
		EXIT_FRAME_THREAD(gRawSock);
	}
#endif

	CW_REPEAT_FOREVER {

		n = recvfrom(gRawSock, buffer, sizeof(buffer), 0, NULL, NULL);

		if (n < 0)
			continue;

		if (!wtpInRunState) {
			CWLog("WTP is not in RUN state");
			continue;
		}

		CW_CREATE_OBJECT_ERR(listElement, CWBindingDataListElement, EXIT_FRAME_THREAD(gRawSock);
		    );

		if (gWTPTunnelMode == CW_TUNNEL_MODE_802_DOT_11_TUNNEL) {
			encaps_len = from_8023_to_80211(buffer, n, buf80211, macAddr);

			if (!extract802_11_Frame(&listElement->frame, buf80211, encaps_len)) {
				CWDebugLog("THR FRAME: Error extracting a frame");
				EXIT_FRAME_THREAD(gRawSock);
			}

			listElement->frame->data_msgType = CW_IEEE_802_11_FRAME_TYPE;

			CWDebugLog("Recv 802.11 data(len:%d) from %s", encaps_len, gRadioInterfaceName_0);
		} else {
			if (!extract802_11_Frame(&listElement->frame, buffer, n)) {
				CWDebugLog("THR FRAME: Error extracting a frame");
				EXIT_FRAME_THREAD(gRawSock);
			}

			listElement->frame->data_msgType = CW_IEEE_802_3_FRAME_TYPE;

			CWDebugLog("Recv 802.3 data(len:%d) from %s", n, gRadioInterfaceName_0);
		}

		listElement->bindingValues = NULL;

		CWLockSafeList(gFrameList);
		CWAddElementToSafeListTail(gFrameList, listElement, sizeof(CWBindingDataListElement));
		CWUnlockSafeList(gFrameList);
	}

	close(gRawSock);
	return (NULL);
}
