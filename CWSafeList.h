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

 
#ifndef __CAPWAP_CWSafeList_HEADER__
#define __CAPWAP_CWSafeList_HEADER__

#include "CWThread.h"

typedef void* CWSafeList;

typedef struct _CWPrivateSafeElement
{
	void* pData;
	int nSize;
	CWBool dataFlag;
	struct _CWPrivateSafeElement* pPrev;
	struct _CWPrivateSafeElement* pNext;
} CWPrivateSafeElement;

typedef struct _CWPrivateSafeList
{
	CWThreadMutex* pThreadMutex;
	CWThreadCondition* pThreadCond;

	unsigned long nCount;
	CWPrivateSafeElement* pFirstElement;
	CWPrivateSafeElement* pLastElement;
} CWPrivateSafeList;

CWBool CWCreateSafeList(CWSafeList* pSafeList);
void CWDestroySafeList(CWSafeList safeList);

void CWSetMutexSafeList(CWSafeList safeList, CWThreadMutex* pThreadMutex);
void CWSetConditionSafeList(CWSafeList safeList, CWThreadCondition* pThreadCond);

CWBool CWLockSafeList(CWSafeList safeList);
void CWUnlockSafeList(CWSafeList safeList);
CWBool CWWaitElementFromSafeList(CWSafeList safeList);
CWBool CWSignalElementSafeList(CWSafeList safeList);

unsigned long CWGetCountElementFromSafeList(CWSafeList safeList);
CWBool CWAddElementToSafeListHead(CWSafeList safeList, void* pData, int nSize);
void* CWGetHeadElementFromSafeList(CWSafeList safeList, int* pSize);
void* CWRemoveHeadElementFromSafeList(CWSafeList safeList, int* pSize);
void* CWRemoveHeadElementFromSafeListwithDataFlag(CWSafeList safeList, int* pSize,CWBool * dataFlag);
CWBool CWAddElementToSafeListTail(CWSafeList safeList, void* pData, int nSize);
CWBool CWAddElementToSafeListTailwitDataFlag(CWSafeList safeList, void* pData, int nSize,CWBool dataFlag);
void* CWRemoveTailElementFromSafeList(CWSafeList safeList, int* pSize);
void CWCleanSafeList(CWSafeList safeList, void (*deleteFunc)(void *));
 
#endif
 
