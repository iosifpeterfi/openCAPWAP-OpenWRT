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
 * Author :  Sotiraq Sima (Sotiraq.Sima@gmail.com)                                         *
 *                                                                                         *
 *******************************************************************************************/

#include <linux/types.h>

enum {
	ERROR = 0,
	CLOSE = 1,

	PING = 2, //  [2]ping
	PONG = 3,//  [3]pong

	START_WLAN = 4,
	START_WLAN_R = 5,

	STOP_WLAN = 6,
	STOP_WLAN_R = 7,

	SET_FREQ = 8, // FREQ sec_channel_offset ht_enabled channel MODE  ex. "[8]2462 0 0 11 0"
	SET_FREQ_R = 9, // 0 or 1  ex. "[9]0"

	GET_FREQ = 10, // ex. "[10]"
	GET_FREQ_R = 11, // ex. "[11]2462 0 0 11 0"

	SET_FRAG = 12, // Typically the range used for fragmentation threshold is 256-2346 (-1 == off) ex. "[12]2000"
	SET_FRAG_R = 13,// ex. "[13]0"

	GET_FRAG = 14, // ex. "[14]"   (-1 == off)
	GET_FRAG_R = 15, // ex. "[15]2000"

	SET_BITRATE = 16,
	SET_BITRATE_R = 17,

	GET_BITRATE = 18,
	GET_BITRATE_R = 19,

	SET_RTS = 20, // 0-2347 (-1 == off)  ex. "[20]100"  (-1 == off)
	SET_RTS_R = 21,// ex. "[21]0"

	GET_RTS = 22, // ex. "]22]"  (-1 == off)
	GET_RTS_R = 23, // ex. "[23]100"

	SET_TXPOWER = 24,
	SET_TXPOWER_R = 25,

	GET_TXPOWER = 26,
	GET_TXPOWER_R = 27,

	/*
	 * VO - 0  CWMIN:3  CWMAX:7  AIFS:2
	 * VI - 1  CWMIN:7  CWMAX:15  AIFS:2
	 * BE - 2  CWMIN:15  CWMAX:1023  AIFS:3
	 * BK - 3  CWMIN:15  CWMAX:1023  AIFS:7
	 */
	SET_TXQ = 28,
	SET_TXQ_R = 29,

	GET_TXQ = 30,
	GET_TXQ_R = 31,

	SET_ADDR = 32,
	SET_ADDR_R = 33,

	DEL_ADDR = 34,
	DEL_ADDR_R = 35,

	ADD_WLAN = 36,
	ADD_WLAN_R = 37,

	DEL_WLAN = 38,
	DEL_WLAN_R = 39,

	WTPRINFO = 40,
	WTPRINFO_R = 41,

	GET_RATES = 42,
	GET_RATES_R = 43,

	GET_MDC = 44,
	GET_MDC_R = 45,

	SET_WTPRINFO = 46,
	SET_WTPRINFO_R = 47,

	SET_RATES = 48,
	SET_RATES_R = 49,

	SET_MDC = 50,
	SET_MDC_R = 51,

	GOLIVE = 52,
	GOLIVE_R = 53,

	WANT_GOLIVE = 54,
	HAVE_TO_WAIT = 55,

	GET_MAC = 56,
	GET_MAC_R = 57,

	SET_MAC = 58,
	SET_MAC_R = 59,

	DATE_TO_WTP = 100,
	DATE_TO_AC = 101,

	CONNECT = 102,
	CONNECT_R = 103,

	GOWAITWLAN = 104,
	GOWAITWLAN_R = 105
};
