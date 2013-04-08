/*******************************************************************************************
 * Copyright (c) 2008 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
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
 * Author : Daniele De Sanctis (danieledesanctis@gmail.com)                                *
 *                                            						   *
 *******************************************************************************************/

#ifndef __CAPWAP_WTPStatsReceive_HEADER__
#define __CAPWAP_WTPStatsReceive_HEADER__

#include "CWWTP.h"
#include <ctype.h>
#include <netinet/in.h>
#include <sys/un.h>


#define SOCKET_PATH     "/tmp/wtp"
#define PACKET_SIZE						65536





#define EXIT_THREAD		CWLog("ERROR Handling Stats: application will be closed!");		\
				close(sock);								\
				exit(1);


//////////////////// copy of ath_clone.h ////////////////////////////////////////////////////////
/*
 * Ioctl-related defintions for the Atheros Wireless LAN controller driver.
 */

#ifndef WME_AC_BE
	#define WME_AC_BE						0
#endif
#ifndef WME_AC_BK
	#define WME_AC_BK						1
#endif
#ifndef WME_AC_VI
	#define WME_AC_VI						2
#endif
#ifndef WME_AC_VO
	#define WME_AC_VO						3
#endif

#ifndef WME_NUM_AC
	#define WME_NUM_AC						4
#endif

#ifndef u_int32_t
#define u_int32_t			unsigned long
#endif

#ifndef u_int16_t
#define u_int16_t			unsigned short
#endif

#ifndef u_int8_t
#define u_int8_t			unsigned char
#endif

#ifndef int8_t
#define int8_t				char
#endif

struct ath_stats_wme_clone {
	u_int32_t tx_packetscount;
	u_int32_t tx_packetslen;
	u_int32_t rx_packetscount;
	u_int32_t rx_packetslen;
	u_int32_t tx_shortretry;	/* tx on-chip retries (short) */
	u_int32_t tx_longretry;		/* tx on-chip retries (long) */
	u_int32_t tx_pending;		/* tx packet pending */
	u_int32_t tx_queue;		/* tx packet queue */
	u_int32_t tx_totalqueued;	/* tx packet total queued */
	u_int32_t tx_failed;		/* tx packet failed */
	u_int32_t tx_discard;		/* tx packet discard */
	u_int8_t wme_chan_logcwmin;	/* cwmin in exponential form */
	u_int8_t wme_chan_logcwmax;	/* cwmax in exponential form */
	u_int8_t wme_chan_aifsn;	/* AIFSN parameters */
	u_int16_t wme_chan_txopLimit;	/* txopLimit */
	u_int8_t wme_bss_logcwmin;	/* cwmin in exponential form */
	u_int8_t wme_bss_logcwmax;	/* cwmax in exponential form */
	u_int8_t wme_bss_aifsn;		/* AIFSN parameters */
	u_int16_t wme_bss_txopLimit;	/* txopLimit */
};

struct ath_stats_clone {
	u_int32_t ast_watchdog;		/* device reset by watchdog */
	u_int32_t ast_hardware;		/* fatal hardware error interrupts */
	u_int32_t ast_bmiss;		/* beacon miss interrupts */
	u_int32_t ast_rxorn;		/* rx overrun interrupts */
	u_int32_t ast_rxeol;		/* rx eol interrupts */
	u_int32_t ast_txurn;		/* tx underrun interrupts */
	u_int32_t ast_mib;		/* mib interrupts */
	u_int32_t ast_tx_packets;	/* packet sent on the interface */
	u_int32_t ast_tx_bytes;		/* total bytes transmitted */
	u_int32_t ast_tx_mgmt;		/* management frames transmitted */
	u_int32_t ast_tx_discard;	/* frames discarded prior to assoc */
	u_int32_t ast_tx_invalid;	/* frames discarded due to is device gone */
	u_int32_t ast_tx_qstop;		/* tx queue stopped because it's full */
	u_int32_t ast_tx_encap;		/* tx encapsulation failed */
	u_int32_t ast_tx_nonode;		/* tx failed due to of no node */
	u_int32_t ast_tx_nobuf;		/* tx failed due to of no tx buffer (data) */
	u_int32_t ast_tx_nobufmgt;	/* tx failed due to of no tx buffer (mgmt)*/
	u_int32_t ast_tx_xretries;	/* tx failed due to of too many retries */
	u_int32_t ast_tx_fifoerr;	/* tx failed due to of FIFO underrun */
	u_int32_t ast_tx_filtered;	/* tx failed due to xmit filtered */
	u_int32_t ast_tx_shortretry;	/* tx on-chip retries (short) */
	u_int32_t ast_tx_longretry;	/* tx on-chip retries (long) */
	u_int32_t ast_tx_badrate;	/* tx failed due to of bogus xmit rate */
	u_int32_t ast_tx_noack;		/* tx frames with no ack marked */
	u_int32_t ast_tx_rts;		/* tx frames with rts enabled */
	u_int32_t ast_tx_cts;		/* tx frames with cts enabled */
	u_int32_t ast_tx_shortpre;	/* tx frames with short preamble */
	u_int32_t ast_tx_altrate;	/* tx frames with alternate rate */
	u_int32_t ast_tx_protect;	/* tx frames with protection */
	u_int32_t ast_rx_orn;		/* rx failed due to of desc overrun */
	u_int32_t ast_rx_crcerr;		/* rx failed due to of bad CRC */
	u_int32_t ast_rx_fifoerr;	/* rx failed due to of FIFO overrun */
	u_int32_t ast_rx_badcrypt;	/* rx failed due to of decryption */
	u_int32_t ast_rx_badmic;		/* rx failed due to of MIC failure */
	u_int32_t ast_rx_phyerr;		/* rx PHY error summary count */
	u_int32_t ast_rx_phy[32];	/* rx PHY error per-code counts */
	u_int32_t ast_rx_tooshort;	/* rx discarded due to frame too short */
	u_int32_t ast_rx_toobig;		/* rx discarded due to frame too large */
	u_int32_t ast_rx_nobuf;		/* rx setup failed due to of no skbuff */
	u_int32_t ast_rx_packets;	/* packet recv on the interface */
	u_int32_t ast_rx_bytes;		/* total bytes received */
	u_int32_t ast_rx_mgt;		/* management frames received */
	u_int32_t ast_rx_ctl;		/* control frames received */
	int8_t ast_tx_rssi;		/* tx rssi of last ack */
	int8_t ast_rx_rssi;		/* rx rssi from histogram */
	u_int32_t ast_be_xmit;		/* beacons transmitted */
	u_int32_t ast_be_nobuf;		/* no skbuff available for beacon */
	u_int32_t ast_per_cal;		/* periodic calibration calls */
	u_int32_t ast_per_calfail;	/* periodic calibration failed */
	u_int32_t ast_per_rfgain;	/* periodic calibration rfgain reset */
	u_int32_t ast_rate_calls;	/* rate control checks */
	u_int32_t ast_rate_raise;	/* rate control raised xmit rate */
	u_int32_t ast_rate_drop;		/* rate control dropped xmit rate */
	u_int32_t ast_ant_defswitch;	/* rx/default antenna switches */
	u_int32_t ast_ant_txswitch;	/* tx antenna switches */
	u_int32_t ast_ant_rx[8];		/* rx frames with antenna */
	u_int32_t ast_ant_tx[8];		/* tx frames with antenna */
	struct ath_stats_wme_clone ast_wme[WME_NUM_AC];
};


//////////////////// copy of ath-monitor.h ////////////////////////////////////////////////////////

/* Monitoraggio dei dati dell'interfaccia di rete */
typedef struct _MM_MONITOR_DATA
{
	int nInterfaceId;
	struct timeval timeData;
	unsigned long nInputFrame;
	unsigned long nInputBandwidth;
	unsigned long nOutputFrame;
	unsigned long nOutputBandwidth;
	struct ath_stats_clone athCurrentStats;
} MM_MONITOR_DATA;

int create_data_Frame(CWProtocolMessage** frame,  char* buffer, int len);
#endif
