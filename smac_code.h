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
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

u8 ERROR = 0;

u8 CLOSE = 1;

u8 PING = 2; //  [2]ping
u8 PONG = 3;//  [3]pong

/* only WTP */

u8 START_WLAN = 4;
u8 START_WLAN_R = 5;

u8 STOP_WLAN = 6;
u8 STOP_WLAN_R = 7;

u8 SET_FREQ = 8; // FREQ sec_channel_offset ht_enabled channel MODE  ex. "[8]2462 0 0 11 0"
u8 SET_FREQ_R = 9; // 0 or 1  ex. "[9]0"

u8 GET_FREQ = 10; // ex. "[10]"
u8 GET_FREQ_R = 11; // ex. "[11]2462 0 0 11 0"

u8 SET_FRAG = 12; // Typically the range used for fragmentation threshold is 256-2346 (-1 == off) ex. "[12]2000"
u8 SET_FRAG_R = 13;// ex. "[13]0"

u8 GET_FRAG = 14; // ex. "[14]"   (-1 == off)
u8 GET_FRAG_R = 15; // ex. "[15]2000" 

u8 SET_BITRATE = 16; 
u8 SET_BITRATE_R = 17;

u8 GET_BITRATE = 18; 
u8 GET_BITRATE_R = 19; 

u8 SET_RTS = 20; // 0-2347 (-1 == off)  ex. "[20]100"  (-1 == off)
u8 SET_RTS_R = 21;// ex. "[21]0"

u8 GET_RTS = 22; // ex. "]22]"  (-1 == off)
u8 GET_RTS_R = 23; // ex. "[23]100"

u8 SET_TXPOWER = 24; 
u8 SET_TXPOWER_R = 25;

u8 GET_TXPOWER = 26; 
u8 GET_TXPOWER_R = 27;


/*
 * VO - 0  CWMIN:3  CWMAX:7  AIFS:2
 * VI - 1  CWMIN:7  CWMAX:15  AIFS:2
 * BE - 2  CWMIN:15  CWMAX:1023  AIFS:3
 * BK - 3  CWMIN:15  CWMAX:1023  AIFS:7
 */
u8 SET_TXQ = 28; 
u8 SET_TXQ_R = 29; 

u8 GET_TXQ = 30; 
u8 GET_TXQ_R = 31;


u8 SET_ADDR = 32;
u8 SET_ADDR_R = 33;

u8 DEL_ADDR = 34;
u8 DEL_ADDR_R = 35;




u8 ADD_WLAN = 36;
u8 ADD_WLAN_R = 37;

u8 DEL_WLAN = 38;
u8 DEL_WLAN_R = 39;


u8 WTPRINFO = 40;
u8 WTPRINFO_R = 41;

u8 GET_RATES = 42;
u8 GET_RATES_R = 43;

u8 GET_MDC = 44;
u8 GET_MDC_R = 45;



u8 SET_WTPRINFO = 46;
u8 SET_WTPRINFO_R = 47;

u8 SET_RATES = 48;
u8 SET_RATES_R = 49;

u8 SET_MDC = 50;
u8 SET_MDC_R = 51;

u8 GOLIVE = 52;
u8 GOLIVE_R = 53;


u8 WANT_GOLIVE = 54;
u8 HAVE_TO_WAIT = 55;


u8 GET_MAC = 56;
u8 GET_MAC_R = 57;

u8 SET_MAC = 58;
u8 SET_MAC_R = 59;

//--------

u8 DATE_TO_WTP = 100;
u8 DATE_TO_AC = 101;

u8 CONNECT = 102;
u8 CONNECT_R = 103;


u8 GOWAITWLAN = 104;
u8 GOWAITWLAN_R = 105;


