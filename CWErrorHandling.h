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


#ifndef __CAPWAP_CWErrorHandling_HEADER__
#define __CAPWAP_CWErrorHandling_HEADER__

typedef enum {
	CW_ERROR_SUCCESS = 1,
	CW_ERROR_OUT_OF_MEMORY,
	CW_ERROR_WRONG_ARG,
	CW_ERROR_INTERRUPTED,
	CW_ERROR_NEED_RESOURCE,
	CW_ERROR_COMUNICATING,
	CW_ERROR_CREATING,
	CW_ERROR_GENERAL,
	CW_ERROR_OPERATION_ABORTED,
	CW_ERROR_SENDING,
	CW_ERROR_RECEIVING,
	CW_ERROR_INVALID_FORMAT,
	CW_ERROR_TIME_EXPIRED,
	CW_ERROR_NONE
} CWErrorCode;


typedef struct {
	CWErrorCode code;
	char message[256];
	int line;
	char fileName[64];
} CWErrorHandlingInfo;

#define CWErrorRaiseSystemError(error)		{					\
							char buf[256];			\
							strerror_r(errno, buf, 256);	\
							CWErrorRaise(error, buf);	\
							return CW_FALSE;		\
						}
											
#define CWErrorRaise(code, msg) 		_CWErrorRaise(code, msg, __FILE__, __LINE__)
#define CWErr(arg)				((arg) || _CWErrorHandleLast(__FILE__, __LINE__))
#define CWErrorHandleLast()			_CWErrorHandleLast(__FILE__, __LINE__)

CWBool _CWErrorRaise(CWErrorCode code, const char *msg, const char *fileName, int line);
void CWErrorPrint(CWErrorHandlingInfo *infoPtr, const char *desc, const char *fileName, int line);
CWErrorCode CWErrorGetLastErrorCode(void);
CWBool _CWErrorHandleLast(const char *fileName, int line);

#endif
