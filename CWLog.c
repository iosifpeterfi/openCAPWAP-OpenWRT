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
//#define WRITE_STD_OUTPUT 1 

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

static FILE *gLogFile = NULL;

#ifndef CW_SINGLE_THREAD
	CWThreadMutex gFileMutex;
#endif

void CWLogInitFile(char *fileName) {
	if(fileName == NULL) {
		CWLog("Wrong File Name for Log File");
	}
	if((gLogFile = fopen(fileName, "w")) == NULL) {
		CWLog("Can't open log file: %s", strerror(errno));
		exit(1);
	}
	#ifndef CW_SINGLE_THREAD
		if(!CWCreateThreadMutex(&gFileMutex)) {
			CWLog("Can't Init File Mutex for Log");
			exit(1);
		}
	#endif
}


CWBool checkResetFile(){
	long fileSize=0;

	if((fileSize=ftell(gLogFile))==-1)
	{
		CWLog("An error with log file occurred: %s", strerror(errno));
		return 0;
	}
	if (fileSize>=gMaxLogFileSize)
	{
		fclose(gLogFile);
		if((gLogFile = fopen(gLogFileName, "w")) == NULL) 
		{
			CWLog("Can't open log file: %s", strerror(errno));
			return 0;
		}
	}
	return 1;
}


void CWLogCloseFile() {
	#ifndef CW_SINGLE_THREAD
		CWDestroyThreadMutex(&gFileMutex);
	#endif
	
	fclose(gLogFile);
}

__inline__ void CWVLog(const char *format, va_list args) {
	char *logStr = NULL;
	time_t now;
	char *nowReadable = NULL;
		
	if(format == NULL) return;
	
	now = time(NULL);
	nowReadable = ctime(&now);
	
	nowReadable[strlen(nowReadable)-1] = '\0';
	
	// return in case of memory err: we're not performing a critical task
	CW_CREATE_STRING_ERR(logStr, (strlen(format)+strlen(nowReadable)+100), return;);
	
	//sprintf(logStr, "[CAPWAP::%s]\t\t %s\n", nowReadable, format);
	sprintf(logStr, "[CAPWAP::%s]\t%08x\t %s\n", nowReadable, (unsigned int)CWThreadSelf(), format);

	if(gLogFile != NULL) {
		char fileLine[256];
		
		#ifndef CW_SINGLE_THREAD
			CWThreadMutexLock(&gFileMutex);
			fseek(gLogFile, 0L, SEEK_END);
		#endif
		
		vsnprintf(fileLine, 255, logStr, args);
	
		if(!checkResetFile()) 
		{
			CWThreadMutexUnlock(&gFileMutex);
			exit (1);
		}
		
		fwrite(fileLine, strlen(fileLine), 1, gLogFile);
		fflush(gLogFile);
		
		#ifndef CW_SINGLE_THREAD
			CWThreadMutexUnlock(&gFileMutex);
		#endif
	}
#ifdef WRITE_STD_OUTPUT
	vprintf(logStr, args);
#endif	
	
	CW_FREE_OBJECT(logStr);
}

__inline__ void CWLog(const char *format, ...) {
	va_list args;
	
	va_start(args, format);
	if (gEnabledLog)
		{CWVLog(format, args);}
	va_end(args);
}

__inline__ void CWDebugLog(const char *format, ...) {
	#ifdef CW_DEBUGGING
		char *logStr = NULL;
		va_list args;
		time_t now;
		char *nowReadable = NULL;
		
		if (!gEnabledLog) {return;}

		if(format == NULL) {
#ifdef WRITE_STD_OUTPUT
			printf("\n");
#endif
			return;
		}
		
		now = time(NULL);
		nowReadable = ctime(&now);
		
		nowReadable[strlen(nowReadable)-1] = '\0';
		
		// return in case of memory err: we're not performing a critical task
		CW_CREATE_STRING_ERR(logStr, (strlen(format)+strlen(nowReadable)+100), return;);
		
		//sprintf(logStr, "[[CAPWAP::%s]]\t\t %s\n", nowReadable, format);
		sprintf(logStr, "[CAPWAP::%s]\t%08x\t %s\n", nowReadable, (unsigned int)CWThreadSelf(), format);

		va_start(args, format);
		
		if(gLogFile != NULL) {
			char fileLine[256];
			
			#ifndef CW_SINGLE_THREAD
				CWThreadMutexLock(&gFileMutex);
				fseek(gLogFile, 0L, SEEK_END);
			#endif
			
			vsnprintf(fileLine, 255, logStr, args);

			if(!checkResetFile()) 
			{
				CWThreadMutexUnlock(&gFileMutex);
				exit (1);
			}

			fwrite(fileLine, strlen(fileLine), 1, gLogFile);
			
			fflush(gLogFile);
			
			#ifndef CW_SINGLE_THREAD
			CWThreadMutexUnlock(&gFileMutex);
			#endif
		}
#ifdef WRITE_STD_OUTPUT	
		vprintf(logStr, args);
#endif
		
		va_end(args);
		CW_FREE_OBJECT(logStr);
	#endif
}
