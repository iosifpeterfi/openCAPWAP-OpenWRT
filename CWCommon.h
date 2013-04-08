/*******************************************************************************************
 * Copyright (c) 2006-9 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
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
 *           Antonio Davoli (antonio.davoli@gmail.com)                                     *
 *******************************************************************************************/

 
#ifndef __CAPWAP_CWCommon_HEADER__
#define __CAPWAP_CWCommon_HEADER__


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#ifdef MACOSX
	#include <netinet/if_ether.h>
#else
	#include <linux/if_ether.h>
#endif
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include "wireless_copy.h"

// make sure the types really have the right sizes
#define CW_COMPILE_TIME_ASSERT(name, x)               typedef int CWDummy_ ## name[(x) * 2 - 1]

// if you get a compile error, change types (NOT VALUES!) according to your system
CW_COMPILE_TIME_ASSERT(int_size, sizeof(int) == 4);
CW_COMPILE_TIME_ASSERT(char_size, sizeof(char) == 1);

#define		CW_BUFFER_SIZE					65536
#define		CW_ZERO_MEMORY					bzero
#define		CW_COPY_MEMORY(dst, src, len)			bcopy(src, dst, len)
#define		CW_REPEAT_FOREVER				while(1)

#define DEFAULT_LOG_SIZE					1000000

typedef enum {
	CW_FALSE = 0,
	CW_TRUE = 1
} CWBool;

typedef enum {
	CW_ENTER_SULKING,
	CW_ENTER_DISCOVERY,
	CW_ENTER_JOIN,
	CW_ENTER_CONFIGURE,
	CW_ENTER_DATA_CHECK,
	CW_ENTER_RUN,
	CW_ENTER_RESET,
	CW_QUIT
} CWStateTransition;

extern const char *CW_CONFIG_FILE;
extern int gCWForceMTU;
extern int gCWRetransmitTimer;
extern int gCWNeighborDeadInterval;
extern int gCWMaxRetransmit; 
extern int gMaxLogFileSize;
extern int gEnabledLog;

#define	CW_FREE_OBJECT(obj_name)		{if(obj_name){free((obj_name)); (obj_name) = NULL;}}
#define	CW_FREE_OBJECTS_ARRAY(ar_name, ar_size)	{int _i = 0; for(_i = ((ar_size)-1); _i >= 0; _i--) {if(((ar_name)[_i]) != NULL){ free((ar_name)[_i]);}} free(ar_name); (ar_name) = NULL; }
#define	CW_PRINT_STRING_ARRAY(ar_name, ar_size)	{int i = 0; for(i = 0; i < (ar_size); i++) printf("[%d]: **%s**\n", i, ar_name[i]);}

// custom error
#define	CW_CREATE_OBJECT_ERR(obj_name, obj_type, on_err)	{obj_name = (obj_type*) (malloc(sizeof(obj_type))); if(!(obj_name)) {on_err}}
#define	CW_CREATE_OBJECT_SIZE_ERR(obj_name, obj_size,on_err)	{obj_name = (malloc(obj_size)); if(!(obj_name)) {on_err}}
#define	CW_CREATE_ARRAY_ERR(ar_name, ar_size, ar_type, on_err)	{ar_name = (ar_type*) (malloc(sizeof(ar_type) * (ar_size))); if(!(ar_name)) {on_err}}
#define	CW_CREATE_STRING_ERR(str_name, str_length, on_err)	{str_name = (char*) (malloc(sizeof(char) * ((str_length)+1) ) ); if(!(str_name)) {on_err}}
#define	CW_CREATE_STRING_FROM_STRING_ERR(str_name, str, on_err)	{CW_CREATE_STRING_ERR(str_name, strlen(str), on_err); strcpy((str_name), str);}

#ifdef CW_DEBUGGING

#define	CW_CREATE_ARRAY_ERR2(ar_name, ar_size, ar_type, on_err)		{ar_name = (ar_type*) (malloc(sizeof(ar_type) * (ar_size))); if((ar_name)) {on_err}}
#define	CW_CREATE_OBJECT_ERR2(obj_name, obj_type, on_err)		{obj_name = (obj_type*) (malloc(sizeof(obj_type))); if((obj_name)) {on_err}}
#define	CW_CREATE_OBJECT_SIZE_ERR2(obj_name, obj_size,on_err)		{obj_name = (malloc(obj_size)); if((obj_name)) {on_err}}
#define	CW_CREATE_STRING_ERR2(str_name, str_length, on_err)		{str_name = (char*) (malloc(sizeof(char) * ((str_length)+1) ) ); if((str_name)) {on_err}}
#define	CW_CREATE_STRING_FROM_STRING_ERR2(str_name, str, on_err)	{CW_CREATE_STRING_ERR2(str_name, strlen(str), on_err); strcpy((str_name), str);}

#endif

#include "CWStevens.h"
#include "config.h"
#include "CWLog.h"
#include "CWErrorHandling.h"

#include "CWRandom.h"
//#include "CWTimer.h"
#include "timerlib.h"
#include "CWThread.h"
#include "CWNetwork.h"
#include "CWList.h"
#include "CWSafeList.h"

#include "CWProtocol.h"
#include "CWSecurity.h"
#include "CWConfigFile.h"

int CWTimevalSubtract(struct timeval *res, const struct timeval *x, const struct timeval *y);
CWBool CWParseSettingsFile();
void CWErrorHandlingInitLib();

#endif
