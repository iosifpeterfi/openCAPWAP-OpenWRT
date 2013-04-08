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

#ifndef CW_SINGLE_THREAD
	CWThreadSpecific gLastError;
	//CWThreadOnce gInitLastErrorOnce = CW_THREAD_ONCE_INIT;
#else
	static CWErrorHandlingInfo *gLastErrorDataPtr;
#endif

void CWErrorHandlingInitLib() {	
	CWDebugLog("Init Errors ");
	
	#ifndef CW_SINGLE_THREAD
		if(!CWThreadCreateSpecific(&gLastError, NULL)) 
		{
			CWLog("Critical Error, closing the process..."); 
			exit(1);
		}
	#else
		CW_CREATE_OBJECT_ERR(infoPtr, CWErrorHandlingInfo, return;);
		infoPtr->code = CW_ERROR_NONE;
		gLastErrorDataPtr = infoPtr;
	#endif
}

CWBool _CWErrorRaise(CWErrorCode code, const char *msg, const char *fileName, int line) {
	CWErrorHandlingInfo *infoPtr;
	
	#ifndef CW_SINGLE_THREAD
		infoPtr = CWThreadGetSpecific(&gLastError);
		if(infoPtr==NULL){
			CW_CREATE_OBJECT_ERR(infoPtr, CWErrorHandlingInfo, exit(1););
			infoPtr->code = CW_ERROR_NONE;
			if(!CWThreadSetSpecific(&gLastError, infoPtr))
			{
				CWLog("Critical Error, closing the process..."); 
				exit(1);
			}
		}
	#else
		infoPtr = gLastErrorDataPtr;
	#endif
	
	if(infoPtr == NULL) 
	{
		CWLog("Critical Error: something strange has happened, closing the process..."); 
		exit(1);
	}
	
	infoPtr->code = code;
	if(msg != NULL) strcpy(infoPtr->message, msg);
	else infoPtr->message[0]='\0';
	if(fileName != NULL) strcpy(infoPtr->fileName, fileName);
	infoPtr->line = line;
	
	return CW_FALSE;
}

void CWErrorPrint(CWErrorHandlingInfo *infoPtr, const char *desc, const char *fileName, int line) {
	if(infoPtr == NULL) return;
	
	if(infoPtr->message != NULL && infoPtr->message[0]!='\0') {
		CWLog("Error: %s. %s .", desc, infoPtr->message);
	} else {
		CWLog("Error: %s", desc);
	}
	CWLog("(occurred at line %d in file %s, catched at line %d in file %s).",
		infoPtr->line, infoPtr->fileName, line, fileName);
}

CWErrorCode CWErrorGetLastErrorCode() {
	CWErrorHandlingInfo *infoPtr;
	
	#ifndef CW_SINGLE_THREAD
		infoPtr = CWThreadGetSpecific(&gLastError);
	#else
		infoPtr = gLastErrorDataPtr;
	#endif
	
	if(infoPtr == NULL) return CW_ERROR_GENERAL;
	
	return infoPtr->code;
}

CWBool _CWErrorHandleLast(const char *fileName, int line) {
	CWErrorHandlingInfo *infoPtr;
	
	#ifndef CW_SINGLE_THREAD
		infoPtr = CWThreadGetSpecific(&gLastError);
	#else
		infoPtr = gLastErrorDataPtr;
	#endif
	
	if(infoPtr == NULL) {
		CWLog("No Error Pending");
		exit((3));
		return CW_FALSE;
	}
	
	#define __CW_ERROR_PRINT(str)	CWErrorPrint(infoPtr, (str), fileName, line)
	
	switch(infoPtr->code) {
		case CW_ERROR_SUCCESS:
		case CW_ERROR_NONE:
			return CW_TRUE;
			break;
			
		case CW_ERROR_OUT_OF_MEMORY:
			__CW_ERROR_PRINT("Out of Memory");
			#ifndef CW_SINGLE_THREAD
				CWExitThread(); // note: we can manage this on per-thread basis: ex. we can
								// kill some other thread if we are a manager thread.
			#else
				exit(1);
			#endif
			break;
			
		case CW_ERROR_WRONG_ARG:
			__CW_ERROR_PRINT("Wrong Arguments in Function");
			break;
			
		case CW_ERROR_NEED_RESOURCE:
			__CW_ERROR_PRINT("Missing Resource");
			break;
			
		case CW_ERROR_GENERAL:
			__CW_ERROR_PRINT("Error Occurred");
			break;
		
		case CW_ERROR_CREATING:
			__CW_ERROR_PRINT("Error Creating Resource");
			break;
			
		case CW_ERROR_SENDING:
			__CW_ERROR_PRINT("Error Sending");
			break;
		
		case CW_ERROR_RECEIVING:
			__CW_ERROR_PRINT("Error Receiving");
			break;
			
		case CW_ERROR_INVALID_FORMAT:
			__CW_ERROR_PRINT("Invalid Format");
			break;
				
		case CW_ERROR_INTERRUPTED:
		default:
			break;
	}
	
	return CW_FALSE;
}
