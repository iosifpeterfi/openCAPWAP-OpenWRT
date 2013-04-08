/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	   *
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

FILE *gCWConfigFile = NULL;

CWConfigValue 	*gConfigValues;
int 		gConfigValuesCount;

/*
 * Replacement for std fgets which seems to dislike windows return character
 */
char *CWFgets(char *buf, int bufSize, FILE *f) {
	int i = -1;
	
	if(buf == NULL || f == NULL || bufSize <= 0) return NULL;
	
	CW_ZERO_MEMORY(buf, bufSize);
	
	do {
		i++;
		buf[i] = getc(f);
		if(buf[i] == EOF) {
			i--;
			break;
		}
	} while (i < bufSize && buf[i] != '\n' && buf[i] != '\r');
	
	if(i == -1) return NULL;
	i++;
	buf[i] = '\0';
	
	return buf;
}

/* 
 * Get one "useful" (not a comment, not blank) line from the config file 
 */
char * CWGetCommand(FILE *configFile) {

	char *buff = NULL;
	char *command = NULL;
	char *ret = NULL;

	CW_CREATE_STRING_ERR(buff, CW_BUFFER_SIZE, return NULL;);
	
	/* skip comments and blank lines */
	while ( ((ret = CWFgets(buff, CW_BUFFER_SIZE, configFile)) != NULL) &&\
		(buff[0] == '\n' || buff[0] == '\r' || buff[0] == '#') );
	
	if(buff != NULL && ret != NULL) {

		int len = strlen(buff);
		buff[len-1] = '\0'; /* remove newline */
		
		CW_CREATE_STRING_ERR(command, len-1, return NULL;);
		strcpy(command, buff);
	}
	
	CW_FREE_OBJECT(buff);
	
	return command;
}

/*
 * Parses the configuration file. 
 *
 * Params: isCount	CW_TRUE to just count ACAddresses and paths; 
 * 			CW_FALSE to actually parse them.
 *
 * Return: CW_TRUE if the operation is succesful; CW_FALSE otherwise.
 */
CWBool CWParseTheFile(CWBool isCount) {

	char *line = NULL;
	int i;
	
	if(!isCount) {

		for(i = 0; i < gConfigValuesCount; i++) {
		
			if(gConfigValues[i].type == CW_STRING_ARRAY) {
		
				/* avoid to allocate 0 bytes */
				if (gConfigValues[i].count) {
				
					CW_CREATE_ARRAY_ERR((gConfigValues[i].value.str_array_value),
							     gConfigValues[i].count, char*, 
							     return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				}
			}
		}
	} else {
		
		for(i = 0; i < gConfigValuesCount; i++) {
		
			if(gConfigValues[i].type == CW_STRING_ARRAY) {
		
				gConfigValues[i].count = 0;
			}
		}
	}
	
	gCWConfigFile = fopen (CW_CONFIG_FILE, "rb");
	if (gCWConfigFile == NULL) CWErrorRaiseSystemError(CW_ERROR_GENERAL);
	
	while((line = CWGetCommand(gCWConfigFile)) != NULL) {

		int i, j;
		
		CWDebugLog("*** Parsing (%s) ***", line);
		
		for(i = 0; i < gConfigValuesCount; i++) {
		
			if(!strncmp(line, 
				    gConfigValues[i].code,
				    strlen(gConfigValues[i].code))) {
		
				char *myLine = line + strlen(gConfigValues[i].code);
				
				switch(gConfigValues[i].type) {

					case CW_INTEGER:
						gConfigValues[i].value.int_value = atoi(myLine);
						break;
					case CW_STRING:
						/*
                                                 * BUG - LE02
                                                 * If this function was called just to count ACAddresses and
                                                 * paths, we MUST NOT allocate a string value; the actual allocation
                                                 * will be performed when the function is called with the isCount
                                                 * argument = CW_FALSE.  
                                                 *
                                                 * 19/10/2009 - Donato Capitella
                                                 */
					
						if(!isCount) CW_CREATE_STRING_FROM_STRING_ERR(gConfigValues[i].value.str_value, 
										 myLine, 
										 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
						break;
					case CW_STRING_ARRAY:
						#ifdef CW_DEBUGGING
							CWDebugLog("*** Parsing String Array... *** \n");
						#endif
						j = 0;
						CW_FREE_OBJECT(line);
						while((line = CWGetCommand(gCWConfigFile)) != NULL && strcmp(line, gConfigValues[i].endCode)) {
							#ifdef CW_DEBUGGING
								CWDebugLog("*** Parsing String (%s) *** \n", line);
							#endif
							
							if(isCount) gConfigValues[i].count++;
							else {
								CW_CREATE_STRING_FROM_STRING_ERR((gConfigValues[i].value.str_array_value)[j], 
												 line, 
												 return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
								j++;
							}
							CW_FREE_OBJECT(line);
						}
						break;
				}
				break;
			}
		}
		CW_FREE_OBJECT(line);
	}

	CWDebugLog("*** Config File Parsed ***");
	fclose(gCWConfigFile);
	
	return CW_TRUE;
}

/* parses the configuration file */
CWBool CWParseConfigFile() {

	if( !(CWConfigFileInitLib()) ) return CW_FALSE;

	/* just count the objects */
	if( !CWParseTheFile(CW_TRUE) ) return CW_FALSE;
	
	/* actually parse */
	if( !CWParseTheFile(CW_FALSE) ) return CW_FALSE;
	
	#ifdef CW_DEBUGGING
		{
			int i;
			for(i = 0; i < gConfigValuesCount; i++) {

				if(gConfigValues[i].type == CW_INTEGER) {
				
					CWLog("%s%d", 
					      gConfigValues[i].code,
					      gConfigValues[i].value.int_value);
				}
			}
		}
		CWDebugLog("*** Config File END ***");
	#endif
	
	return CWConfigFileDestroyLib();
}

