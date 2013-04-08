/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica	*
 *                          Universita' Campus BioMedico - Italy								*
 *																								*
 * This program is free software; you can redistribute it and/or modify it under the terms		*
 * of the GNU General Public License as published by the Free Software Foundation; either		*
 * version 2 of the License, or (at your option) any later version.								*
 *																								*
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY				*
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A				*
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.						*
 *																								*
 * You should have received a copy of the GNU General Public License along with this			*
 * program; if not, write to the:																*
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,							*
 * MA  02111-1307, USA.																			*
 *																								*
 * -------------------------------------------------------------------------------------------- *
 * Project:  Capwap																				*
 *																								*
 * Authors : Ludovico Rossi (ludo@bluepixysw.com)												*  
 *           Del Moro Andrea (andrea_delmoro@libero.it)											*
 *           Giovannini Federica (giovannini.federica@gmail.com)								*
 *           Massimo Vellucci (m.vellucci@unicampus.it)											*
 *           Mauro Bisson (mauro.bis@gmail.com)													*
 *	         Antonio Davoli (antonio.davoli@gmail.com)											*
 ************************************************************************************************/


#ifndef __CAPWAP_ACBinding_HEADER__
#define __CAPWAP_ACBinding_HEADER__

CWBool CWACInitBinding(int i);
CWBool CWBindingAssembleConfigureResponse(CWProtocolMessage **msgElems, int *msgElemCountPtr);
/****************************************************
 * 2009 Update:										*
 * The field BindingMsgElement has been added for	*
 * the multiple type of Message Element.			*
 ****************************************************/
CWBool CWBindingAssembleConfigurationUpdateRequest(CWProtocolMessage **msgElems, int *msgElemCountPtr, int BindingMsgElement);
CWBool CWBindingSaveConfigurationUpdateResponse(CWProtocolResultCode resultCode, int WTPIndex);

#endif
