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


#ifndef __CAPWAP_CWConfigFile_HEADER__
#define __CAPWAP_CWConfigFile_HEADER__
 
typedef char **CWStringArray;

typedef struct {
	enum {
		CW_INTEGER,
		CW_STRING,
		CW_STRING_ARRAY
	} type;
	
	union {
		int int_value;
		char *str_value;
		char **str_array_value;
	} value;
	
	char *code;
	char *endCode;
	
	int count;
} CWConfigValue;

extern CWConfigValue *gConfigValues;
extern int gConfigValuesCount;


CWBool CWParseConfigFile();
char * CWGetCommand(FILE *configFile);
CWBool CWConfigFileInitLib(void);
CWBool CWConfigFileDestroyLib(void);

#endif
