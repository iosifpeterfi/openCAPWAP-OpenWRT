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
#include <pcap.h>

#include "CWWTP.h"

#define EXIT_THREAD		CWLog("ERROR Handling Frames: application will be closed!");		\
				exit(1);



#define WL_DEVICE "wl0"
#define MAC_ADDR_LEN		6

/* check this magic number */
#define WLC_IOCTL_MAGIC		0x14e46c77

#define WLC_GET_MAGIC				0
#define WLC_GET_BSSID				23
#define WLC_SET_BSSID				24
#define WLC_GET_SSID				25
#define WLC_SET_SSID				26
#define WLC_GET_CHANNEL				29
#define WLC_SET_CHANNEL				30
#define WLC_GET_MONITOR				107     /* discovered by nbd */
#define WLC_SET_MONITOR				108     /* discovered by nbd */
#define WLC_GET_AP				117
#define WLC_SET_AP				118
#define WLC_GET_RSSI				127
#define WLC_GET_ASSOCLIST			159

#ifdef DEFINE_TYPES
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef unsigned int u_int;
#endif

typedef enum {
  mgt_assocRequest = 0,
  mgt_assocResponse = 1,
  mgt_reassocRequest = 2,
  mgt_reassocResponse = 3,
  mgt_probeRequest = 4,
  mgt_probeResponse = 5,
  mgt_beacon = 8,
  mgt_disassoc = 10,
  mgt_auth = 11,
  mgt_deauth = 12
  } wifi_frametype;

typedef struct ieee802_11_hdr {
  u_char frame_control;
  u_char flags;
#define IEEE80211_TO_DS 0x01
#define IEEE80211_FROM_DS 0x02
#define IEEE80211_MORE_FRAG 0x04
#define IEEE80211_RETRY 0x08
#define IEEE80211_PWR_MGT 0x10
#define IEEE80211_MORE_DATA 0x20
#define IEEE80211_WEP_FLAG 0x40
#define IEEE80211_ORDER_FLAG 0x80
  u_short duration;
  u_char addr1[6];
  u_char addr2[6];
  u_char addr3[6];
  u_short frag_and_seq;
  } ieee802_11_hdr;

typedef struct {
  u_char timestamp[8];
  u_short bcn_interval;
  u_short caps;
#define MGT_CAPS_AP 0x1
#define MGT_CAPS_IBSS 0x2
#define MGT_CAPS_WEP 0x10
  } ieee_802_11_mgt_frame;



typedef struct prism_hdr {
  u_int msg_code;
  u_int msg_length;
  char cap_device[16];
  } prism_hdr;

typedef struct prism_did {
  u_short did;
  u_short status1;
  u_short status2;
  u_short length;

  } prism_did;

typedef enum prism_did_num {
  pdn_host_time = 0x1041,
  pdn_mac_time = 0x2041,
  pdn_rssi = 0x4041,
  pdn_sq = 0x5041,
  pdn_datarate = 0x8041,
  pdn_framelen = 0xa041
  } prism_did_num;


void print_mac(u_char * mac, char * extra);
void fprint_mac(FILE * outf, u_char * mac, char * extra);
int macAddrCmp (unsigned char* addr1, unsigned char* addr2);
int get_mac(u_char *buf);
void dealWithPacket(struct pcap_pkthdr * header, const u_char * packet, u_char * wl0_mac_address);
#endif
