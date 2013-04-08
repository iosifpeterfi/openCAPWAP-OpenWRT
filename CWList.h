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

 
#ifndef __CAPWAP_CWList_HEADER__
#define __CAPWAP_CWList_HEADER__

#define CW_LIST_INIT		NULL

typedef struct _s {
	void *data;
	struct _s *next;
} CWListElement;

typedef enum {
	CW_LIST_ITERATE_RESET,
	CW_LIST_ITERATE
} CWListIterateMode;

typedef CWListElement *CWList;

CWBool CWAddElementToList(CWList *list, void *element);
CWBool CWAddElementToListTail(CWList *list, void *element);
CWList CWListGetFirstElem(CWList *list);
void *CWListGetNext(CWList list, CWListIterateMode mode);
void *CWSearchInList(CWList list, void *baseElement, CWBool (*compareFunc)(void *, void *));
void *CWDeleteInList(CWList *list, void *baseElement, CWBool (*compareFunc)(void *, void *));
void CWDeleteList(CWList *list, void (*deleteFunc)(void *));
int CWCountElementInList(CWList list);
//CWList * FindLastElementInList (CWList list);

#endif
