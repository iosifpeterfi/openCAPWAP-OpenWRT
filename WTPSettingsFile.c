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


#include "CWCommon.h"

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#define CW_SETTINGS_FILE 	"settings.wtp.txt"

FILE* gSettingsFile=NULL;
char* gInterfaceName=NULL;
char* gEthInterfaceName=NULL;
char* gRadioInterfaceName_0=NULL;
char* gBaseMACInterfaceName=NULL;
char  gBoardReversionNo;

int gHostapd_port;
char*  gHostapd_unix_path;

void CWExtractValue(char* start, char** startValue, char** endValue, int* offset)
{
	*offset=strspn (start+1, " \t\n\r");
	*startValue = start +1+ *offset;

	*offset=strcspn (*startValue, " \t\n\r");
	*endValue = *startValue + *offset -1;
}

CWBool CWParseSettingsFile()
{
	char *line = NULL;
		
	gSettingsFile = fopen (CW_SETTINGS_FILE, "rb");
	if (gSettingsFile == NULL) {
		CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	while((line = (char*)CWGetCommand(gSettingsFile)) != NULL) 
	{
		char* startTag=NULL;
		char* endTag=NULL;
		
		if((startTag=strchr (line, '<'))==NULL) 
		{
			CW_FREE_OBJECT(line);
			continue;
		}

		if((endTag=strchr (line, '>'))==NULL) 
		{
			CW_FREE_OBJECT(line);
			continue;
		}
			
		if (!strncmp(startTag+1, "IF_NAME", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;

			CWExtractValue(endTag, &startValue, &endValue, &offset);

			CW_CREATE_STRING_ERR(gInterfaceName, offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
			strncpy(gInterfaceName, startValue, offset);
			gInterfaceName[offset] ='\0';
			CWLog(": %s", gInterfaceName);
			CW_FREE_OBJECT(line);
			continue;	
		}
		
		if (!strncmp(startTag+1, "WTP_ETH_IF_NAME", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;

			CWExtractValue(endTag, &startValue, &endValue, &offset);

			CW_CREATE_STRING_ERR(gEthInterfaceName, offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
			strncpy(gEthInterfaceName, startValue, offset);
			gEthInterfaceName[offset] ='\0';
			CWLog(": %s", gEthInterfaceName);
			CW_FREE_OBJECT(line);
			continue;	
		}		

		if (!strncmp(startTag+1, "RADIO_0_IF_NAME", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;

			CWExtractValue(endTag, &startValue, &endValue, &offset);

			CW_CREATE_STRING_ERR(gRadioInterfaceName_0, offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
			strncpy(gRadioInterfaceName_0, startValue, offset);
			gRadioInterfaceName_0[offset] ='\0';
			CWLog(": %s", gRadioInterfaceName_0);
			CW_FREE_OBJECT(line);
			continue;	
		}		

		if (!strncmp(startTag+1, "BASE_MAC_IF_NAME", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;

			CWExtractValue(endTag, &startValue, &endValue, &offset);

			CW_CREATE_STRING_ERR(gBaseMACInterfaceName, offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
			strncpy(gBaseMACInterfaceName, startValue, offset);
			gBaseMACInterfaceName[offset] ='\0';
			CWLog(": %s", gBaseMACInterfaceName);
			CW_FREE_OBJECT(line);
			continue;	
		}		

		if (!strncmp(startTag+1, "BOARD_REVISION_NO", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;
			char reversion[16];

			CWExtractValue(endTag, &startValue, &endValue, &offset);
		
			strncpy(reversion, startValue, offset);
			reversion[offset] ='\0';
			gBoardReversionNo = atoi(reversion);
			CWLog(": %d",gBoardReversionNo);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "WTP_HOSTAPD_PORT", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;
			char port_str[16];

			CWExtractValue(endTag, &startValue, &endValue, &offset);
		
			strncpy(port_str, startValue, offset);
			port_str[offset] ='\0';
			gHostapd_port = atoi(port_str);
			CWLog(": %d",gHostapd_port);
			CW_FREE_OBJECT(line);
			continue;		
		}
		if (!strncmp(startTag+1, "WTP_HOSTAPD_UNIX_PATH", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;

			CWExtractValue(endTag, &startValue, &endValue, &offset);

			CW_CREATE_STRING_ERR(gHostapd_unix_path, offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
			strncpy(gHostapd_unix_path, startValue, offset);
			gHostapd_unix_path[offset] ='\0';
			CWLog(": %s", gHostapd_unix_path);
			CW_FREE_OBJECT(line);
			continue;	
		}

		CW_FREE_OBJECT(line);
	}
	return CW_TRUE;
}
