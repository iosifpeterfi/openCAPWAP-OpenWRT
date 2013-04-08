/*******************************************************************************************
 * Copyright (c) 2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
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
 * Author : Antonio Davoli (antonio.davoli@gmail.com)                                      *
 *                                            				                    		   *
 *******************************************************************************************/


#ifndef __CAPWAP_WTPFreqStatsReceive_HEADER__
#define __CAPWAP_WTPFreqStatsReceive_HEADER__

#include "CWWTP.h"
#include <ctype.h>
#include <netinet/in.h>
#include <sys/un.h>

#define SERVER_PORT 1237
#define PACKET_SIZE	65536


#define EXIT_THREAD	CWLog("ERROR Handling Frequency Stats: application will be closed!");		\
				close(recSock);								\
				exit(1);

/* Structs for frequencies' informations */

typedef unsigned char uint8;

#define MAX_FREQ_LENGTH 16
#define MAX_ESSID_LENGTH 32
#define MAX_MAC_ADDR_LENGTH 18 /* XX:XX:XX:XX:XX:XX */

typedef struct _quality {
  uint8 numerator;
  uint8 denominator;
} quality;

typedef struct _FreqQualityInfo {
	uint8 version;
	char ESSID[MAX_ESSID_LENGTH];
	char Address[MAX_MAC_ADDR_LENGTH];
	uint8 channel;
	uint8 assocStations;
	uint8 throughStations;
	quality qualityLevel;
	int signalLevel;
	int noiseLevel;
} FreqQualityInfo;

typedef struct _FREQ_MONITOR_DATA {
  int numberOfCells;
  FreqQualityInfo *qualityOfCells;
} FREQ_MONITOR_DATA;

#endif

