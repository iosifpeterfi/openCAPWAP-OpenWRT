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

#define CW_SETTINGS_FILE 	"settings.ac.txt"

#define CWMIN_DEFAULT	3
#define CWMAX_DEFAULT	10
#define AIFS_DEFAULT	1


FILE* gSettingsFile=NULL;
WTPQosValues* gDefaultQosValues=NULL;
int gHostapd_port;
char* gHostapd_unix_path;

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
	
	CW_CREATE_ARRAY_ERR(gDefaultQosValues, NUM_QOS_PROFILES, WTPQosValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	
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
		
		if (!strncmp(startTag+1, "CWMIN_VOICE", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=CWMIN_DEFAULT;
			gDefaultQosValues[VOICE_QUEUE_INDEX].cwMin = value;
			CWDebugLog("CWMIN_VOICE: %d", gDefaultQosValues[VOICE_QUEUE_INDEX].cwMin);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "CWMAX_VOICE", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=CWMAX_DEFAULT;
			gDefaultQosValues[VOICE_QUEUE_INDEX].cwMax = value;
			CWDebugLog("CWMAX_VOICE: %d", gDefaultQosValues[VOICE_QUEUE_INDEX].cwMax);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "AIFS_VOICE", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=AIFS_DEFAULT;
			gDefaultQosValues[VOICE_QUEUE_INDEX].AIFS = value;
			CWDebugLog("AIFS_VOICE: %d", gDefaultQosValues[VOICE_QUEUE_INDEX].AIFS);
			CW_FREE_OBJECT(line);
			continue;	
		}


		if (!strncmp(startTag+1, "CWMIN_VIDEO", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=CWMIN_DEFAULT;
			gDefaultQosValues[VIDEO_QUEUE_INDEX].cwMin = value;
			CWDebugLog("CWMIN_VIDEO: %d", gDefaultQosValues[VIDEO_QUEUE_INDEX].cwMin);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "CWMAX_VIDEO", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=CWMAX_DEFAULT;
			gDefaultQosValues[VIDEO_QUEUE_INDEX].cwMax = value;
			CWDebugLog("CWMAX_VIDEO: %d", gDefaultQosValues[VIDEO_QUEUE_INDEX].cwMax);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "AIFS_VIDEO", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=AIFS_DEFAULT;
			gDefaultQosValues[VIDEO_QUEUE_INDEX].AIFS = value;
			CWDebugLog("AIFS_VIDEO: %d", gDefaultQosValues[VIDEO_QUEUE_INDEX].AIFS);
			CW_FREE_OBJECT(line);
			continue;	
		}
		

		if (!strncmp(startTag+1, "CWMIN_BEST_EFFORT", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=CWMIN_DEFAULT;
			gDefaultQosValues[BESTEFFORT_QUEUE_INDEX].cwMin = value;
			CWDebugLog("CWMIN_BEST_EFFORT: %d", gDefaultQosValues[BESTEFFORT_QUEUE_INDEX].cwMin);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "CWMAX_BEST_EFFORT", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=CWMAX_DEFAULT;
			gDefaultQosValues[BESTEFFORT_QUEUE_INDEX].cwMax = value;
			CWDebugLog("CWMAX_BEST_EFFORT: %d", gDefaultQosValues[BESTEFFORT_QUEUE_INDEX].cwMax);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "AIFS_BEST_EFFORT", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=AIFS_DEFAULT;
			gDefaultQosValues[BESTEFFORT_QUEUE_INDEX].AIFS = value;
			CWDebugLog("AIFS_BEST_EFFORT: %d", gDefaultQosValues[BESTEFFORT_QUEUE_INDEX].AIFS);
			CW_FREE_OBJECT(line);
			continue;	
		}


		if (!strncmp(startTag+1, "CWMIN_BACKGROUND", endTag-startTag-1))
		{
			int value = atoi(endTag+1);

			if(value==0) value=CWMIN_DEFAULT;
			gDefaultQosValues[BACKGROUND_QUEUE_INDEX].cwMin = value;
			CWDebugLog("CWMIN_BACKGROUND: %d", gDefaultQosValues[BACKGROUND_QUEUE_INDEX].cwMin);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "CWMAX_BACKGROUND", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=CWMAX_DEFAULT;
			gDefaultQosValues[BACKGROUND_QUEUE_INDEX].cwMax = value;
			CWDebugLog("CWMAX_BACKGROUND: %d", gDefaultQosValues[BACKGROUND_QUEUE_INDEX].cwMax);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "AIFS_BACKGROUND", endTag-startTag-1))
		{
			int value = atoi(endTag+1);
			
			if(value==0) value=AIFS_DEFAULT;
			gDefaultQosValues[BACKGROUND_QUEUE_INDEX].AIFS = value;
			CWDebugLog("AIFS_BACKGROUND: %d", gDefaultQosValues[BACKGROUND_QUEUE_INDEX].AIFS);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "AC_HOSTAPD_PORT", endTag-startTag-1))
		{
			
			gHostapd_port = atoi(endTag+1);
			
		    CWDebugLog("Hostapd Port connection: %d", gHostapd_port);
			CW_FREE_OBJECT(line);
			continue;	
		}
		if (!strncmp(startTag+1, "AC_HOSTAPD_UNIX_PATH", endTag-startTag-1))
		{
			char* startValue=NULL;
			char* endValue=NULL;
			int offset = 0;

			CWExtractValue(endTag, &startValue, &endValue, &offset);

			CW_CREATE_STRING_ERR(gHostapd_unix_path, offset, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY,NULL););
			strncpy(gHostapd_unix_path, startValue, offset);
			gHostapd_unix_path[offset] ='\0';
			
			CWDebugLog("Hostapd Unix Domain Path: %s",gHostapd_unix_path);
			CW_FREE_OBJECT(line);
			continue;	
			
		}
		CW_FREE_OBJECT(line);
	}
	return CW_TRUE;
}
