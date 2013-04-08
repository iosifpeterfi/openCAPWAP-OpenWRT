/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica	*
 *                          Universita' Campus BioMedico - Italy								*
 *																								*
 * This program is free software; you can redistribute it and/or modify it under the terms		*
 * of the GNU General Public License as published by the free Software Foundation; either		*
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
 * Authors : Matteo Latini (mtylty@gmail.com)
 *           Donato Capitella (d.capitella@gmail.com)  
 *
 ************************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h> 
#include <signal.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#include "CWWTP.h"
#include "CWVendorPayloads.h"
#include "WUM.h"


CWBool CWParseUCIPayload(CWProtocolMessage *msgPtr, CWVendorUciValues **payloadPtr) {
	int argsLen;
	CWVendorUciValues* uciPayload = NULL; 

	CW_CREATE_OBJECT_ERR(uciPayload, CWVendorUciValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
	uciPayload->command = (unsigned char) CWProtocolRetrieve8(msgPtr);
	uciPayload->response = NULL;
	argsLen = (unsigned int) CWProtocolRetrieve32(msgPtr);	
	if (argsLen != 0) {
		CW_CREATE_STRING_ERR(uciPayload->commandArgs, argsLen, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
		uciPayload->commandArgs = CWProtocolRetrieveStr(msgPtr, argsLen);
	} else 
		uciPayload->commandArgs = NULL;
	
	*payloadPtr = uciPayload;

	CWLog("Parsed UCI Vendor Payload...");
	return CW_TRUE;
}

CWBool CWParseWUMPayload(CWProtocolMessage *msgPtr, CWVendorWumValues **payloadPtr) {
        CWVendorWumValues* wumPayload = NULL;

        CW_CREATE_OBJECT_ERR(wumPayload, CWVendorWumValues, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
        wumPayload->type = (unsigned char) CWProtocolRetrieve8(msgPtr);
       
       	/*
	 * According to the type of the message, retrive additional fields
	 */
	if (wumPayload->type == WTP_UPDATE_REQUEST) {
		wumPayload->_major_v_ = (unsigned char) CWProtocolRetrieve8(msgPtr);
		wumPayload->_minor_v_ = (unsigned char) CWProtocolRetrieve8(msgPtr);
		wumPayload->_revision_v_ = (unsigned char) CWProtocolRetrieve8(msgPtr);
		wumPayload->_pack_size_ = (unsigned int) CWProtocolRetrieve32(msgPtr);
	} else if (wumPayload->type == WTP_CUP_FRAGMENT) {
		wumPayload->_seq_num_ = (unsigned int ) CWProtocolRetrieve32(msgPtr);
		wumPayload->_cup_fragment_size_ = (unsigned int) CWProtocolRetrieve32(msgPtr);
		wumPayload->_cup_ = CWProtocolRetrieveRawBytes(msgPtr, wumPayload->_cup_fragment_size_);
	}

        *payloadPtr = wumPayload;

        CWLog("Parsed WUM Vendor Payload...");
        return CW_TRUE;
}


CWBool CWParseVendorPayload(CWProtocolMessage *msgPtr, int len, CWProtocolVendorSpecificValues* valPtr) {

	CWVendorUciValues *uciPtr;
	CWVendorWumValues *wumPtr;

	CWParseMessageElementStart();

	/*...we choose which payload was used (in this case only
	  uci configuration payloads are used)*/
	valPtr->vendorPayloadType = (unsigned short) CWProtocolRetrieve16(msgPtr);

	switch(valPtr->vendorPayloadType) {
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI:
			if (!(CWParseUCIPayload(msgPtr, &uciPtr)))
			{
				CW_FREE_OBJECT(uciPtr->commandArgs);
				CW_FREE_OBJECT(uciPtr->response);
				CW_FREE_OBJECT(uciPtr);
				return CW_FALSE; // will be handled by the caller
			}
			valPtr->payload = (void *) uciPtr;
			break;
		case CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM:
			if (!(CWParseWUMPayload(msgPtr, &wumPtr)))
			{
				CW_FREE_OBJECT(wumPtr);
				return CW_FALSE; // will be handled by the caller
			}
			valPtr->payload = (void *) wumPtr;
			break;
		default:
			return CW_FALSE; // will be handled by the caller
	}


	return CW_TRUE;
}

/************************************************************************
 * WTP Update System
 ************************************************************************/

/* Update System States */
#define WUM_STATE_WAIT	0	/* waiting for WTP_UPDATE_REQUESTs */
#define WUM_STATE_BUSY	1	/* busy receiving CUP fragments */
#define WUM_STATE_READY	2	/* ready to start the update procedure */

struct WUMState {
	unsigned char state;	/* one of the above states */
	FILE *cupTmp;			/* pointer to the cup file */
	int cupSize;			/* size of the cup file */
	int total_fragments;	/* number of fragments of the cup file */
	int received_fragments;	/* number of fragments already received */
};

struct WUMState wumState = {
	.state = WUM_STATE_WAIT,
	.cupTmp = NULL,
	.cupSize = 0,
	.total_fragments = 0,
	.received_fragments = 0
};

#define CUP_TMP_FILE "/tmp/wtp.cup"
#define WUA_BIN      "WUA"

/*
 * WUMPrepareTmpFile
 *
 * After a WTP_UPDATE_REQUEST, calling this function prepares the update system
 * to accept CUP fragments and reassemble them into a temp file. If something goes
 * wrong, it returns CW_FALSE; otherwise it returns CW_TRUE, the wumState changes 
 * to BUSY and we are ready to store the fragments we've received.
 */
CWBool WUMPrepareForUpdate(int size)
{
	if (wumState.state != WUM_STATE_WAIT) {
		CWLog("Can't start a new update session; the previous one isn't completed yet.\n");
		return CW_FALSE;
	}

	wumState.cupTmp = fopen(CUP_TMP_FILE, "w");
	
	if (wumState.cupTmp == NULL) {
		CWLog("Can't open temp file %s for writing.\n", CUP_TMP_FILE);
		return CW_FALSE;
	}

	wumState.cupSize = size;
	wumState.total_fragments = size / CUP_FRAGMENT_SIZE;
	if (size % CUP_FRAGMENT_SIZE > 0)
		wumState.total_fragments++;
	wumState.received_fragments = 0;
	
	wumState.state = WUM_STATE_BUSY;

	return CW_TRUE;
}

/*
 * When in the BUSY State, this function is used to read and store the
 * fragments that compose the update package (CUP) into a temp file.
 */
CWBool WUMStoreFragment(CWVendorWumValues *wumValues)
{
	int offset, toWrite;

	if (wumState.state != WUM_STATE_BUSY) {
		CWLog("Received an update fragment, but no update session has been initialized yet.\n");
		return CW_FALSE;
	}

	if (wumValues->_seq_num_ > wumState.total_fragments) {
		CWLog("Received an update fragment with invalid sequence number.\n");
		return CW_FALSE;
	}

	offset = wumValues->_seq_num_ * CUP_FRAGMENT_SIZE;

	if (fseek(wumState.cupTmp, offset, SEEK_SET) != 0) {
		CWLog("Can't seek required offset in CUP tmp file.\n");
		return CW_FALSE;
	}

	toWrite = wumValues->_cup_fragment_size_;
	if (fwrite(wumValues->_cup_, 1, toWrite, wumState.cupTmp) != toWrite) {
		CWLog("Error while writing CUP tmp file.\n");
		return CW_FALSE;
	}
	
	CW_FREE_OBJECT(wumValues->_cup_);

	wumState.received_fragments++;
	return CW_TRUE;
}

/*
 * Tells if all fragments of an update have been received and reassembled.
 */
CWBool WUMIsComplete()
{
	if (wumState.state != WUM_STATE_BUSY) {
		return CW_FALSE;
	}

	if (wumState.total_fragments > wumState.received_fragments)
		return CW_FALSE;

	return CW_TRUE;
}

/*
 * Close the temp file and goes back to the READY State.
 */
CWBool WUMCloseFile()
{
	fclose(wumState.cupTmp);
	wumState.cupTmp = NULL;
	wumState.state = WUM_STATE_READY;
	return CW_TRUE;
}

/*
 * Cancel an Update.
 */
CWBool WUMCancel()
{
	wumState.state = WUM_STATE_WAIT;
	if (wumState.cupTmp) fclose(wumState.cupTmp);
	wumState.cupTmp = NULL;
	remove(CUP_TMP_FILE);
	return CW_TRUE;
}


/*
 * CWWTPCheckVersion - checks if we are already updated.
 */
CWBool CWWTPCheckVersion(CWVendorWumValues *wumPayload) 
{
	int ret = CW_FALSE;
	
	if (wumPayload->_major_v_ > WTP_VERSION_MAJOR)
		ret = CW_TRUE;
	else if (wumPayload->_major_v_ == WTP_VERSION_MAJOR){
		if (wumPayload->_minor_v_ > WTP_VERSION_MINOR)
			ret = CW_TRUE;
		else if (wumPayload->_minor_v_ == WTP_VERSION_MINOR){
			if ( wumPayload->_revision_v_ > WTP_VERSION_REVISION )
				ret = CW_TRUE;
		}
	}

	return ret;
}

CWBool StartWUA()
{
    int pid, fd;
    struct flock fl;

    /* The following lock in set just for synchronization purposes */
    fl.l_type   = F_WRLCK; 
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;       
    fl.l_len    = 0;        
    fl.l_pid    = getpid(); 

    fd = open(WTP_LOCK_FILE, O_CREAT | O_WRONLY, S_IRWXU);

    fcntl(fd, F_SETLKW, &fl);  /* F_GETLK, F_SETLK, F_SETLKW */

    pid = fork();
    if (pid == 0) {
        execl(WUA_BIN, WUA_BIN, CUP_TMP_FILE, NULL);
		exit(EXIT_FAILURE);
    } else if (pid < 0){
        CWLog("Can't fork!");
		close(fd);
		return CW_FALSE;
    }

    return CW_TRUE;
}

/*
 * Returns CW_TRUE if the required space is available on the 
 * filesystem where the /tmp dir is mounted.
 */
CWBool check_free_space(int bytes)
{
	struct statvfs diskStat;

	if (statvfs("/tmp", &diskStat) != 0) {
		CWLog("Can't stat filesystem!\n");
		return CW_FALSE;
	}

	if (diskStat.f_bsize * diskStat.f_bfree > bytes)
		return CW_TRUE;
	
	return CW_FALSE;
}

CWBool CWWTPSaveWUMValues(CWVendorWumValues *wumPayload, CWProtocolResultCode *resultCode) 
{	
    /* guards on input values */
	if (wumPayload == NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}

	*resultCode = CW_PROTOCOL_SUCCESS;

	switch(wumPayload->type) {
		case WTP_VERSION_REQUEST:
			wumPayload->type = WTP_VERSION_RESPONSE;
			wumPayload->_major_v_ = WTP_VERSION_MAJOR;
			wumPayload->_minor_v_ = WTP_VERSION_MINOR;
			wumPayload->_revision_v_ = WTP_VERSION_REVISION;
			break;
		case WTP_UPDATE_REQUEST:
			/* Check if update can be performed */
			CWLog("Received Update Request - Version %d.%d.%d",
				wumPayload->_major_v_, wumPayload->_minor_v_, wumPayload->_revision_v_
			);

			wumPayload->type = WTP_UPDATE_RESPONSE;

			if (!CWWTPCheckVersion(wumPayload)) {
				CWLog("WTP already up to date.\n");
				*resultCode = CW_PROTOCOL_FAILURE;	
			} else if (!check_free_space(wumPayload->_pack_size_)) {
				CWLog("No disk space available.\n");
				*resultCode = CW_PROTOCOL_FAILURE;	
			} else if ( WUMPrepareForUpdate(wumPayload->_pack_size_) == CW_FALSE ) {
				*resultCode = CW_PROTOCOL_FAILURE;	
			}

			break;
		case WTP_CUP_FRAGMENT:
			wumPayload->type = WTP_CUP_ACK;
			
			if (!WUMStoreFragment(wumPayload)) {
				*resultCode = CW_PROTOCOL_FAILURE;
			}
			
			/* if this is the last fragment, close temp file and 
			 * pass to the READY state. */
			if (WUMIsComplete()) {
				WUMCloseFile();
			}
			break;
		case WTP_COMMIT_UPDATE:
		    wumPayload->type = WTP_COMMIT_ACK;
		    
			/* Check if we are in the correct state */
			if (wumState.state != WUM_STATE_READY) {
				*resultCode = CW_PROTOCOL_FAILURE;
			}
			
			/* Start the Update Agent */
			StartWUA();
			
			/* Remember to exit after sending the WTP_COMMIT_ACK */
			WTPExitOnUpdateCommit = CW_TRUE;
			break;
		case WTP_CANCEL_UPDATE_REQUEST:
			wumPayload->type = WTP_CANCEL_UPDATE_RESPONSE;

			if (wumState.state == WUM_STATE_BUSY) {
				/* Accepting fragments */
				WUMCloseFile();
				if (!remove(CUP_TMP_FILE)) {
					CWLog("Error while removing cup tmp file.");
				}
			} else if (wumState.state == WUM_STATE_READY) {
				if (!remove(CUP_TMP_FILE)) {
					CWLog("Error while removing cup tmp file.");
				}
			}
			wumState.state = WUM_STATE_WAIT;
	}


	CWLog("Saved WUM Vendor Payload...");
    return CW_TRUE;
}

CWBool CWWTPSaveUCIValues(CWVendorUciValues *uciPayload, CWProtocolResultCode *resultCode) {

	if (uciPayload == NULL) {return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);}

	*resultCode = CW_PROTOCOL_SUCCESS;

	struct sockaddr_in serv_addr;
	int sendSock, slen = sizeof(serv_addr), responseSize, responseCode;
	unsigned int ArgsSize, response = 0, ArgsSizeNet;
	char * bufferMessage;   

	if ((sendSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) ==-1 ) {
		CWLog("[CWSaveUCIValues]: Error on creation of socket.");
		return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}
	
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(UCI_SERVER_PORT);
	
	if ( inet_aton("127.0.0.1", &serv_addr.sin_addr)==0 ) {
	  CWLog("[CWSaveUCIValues]: Error on aton function.");
	  close(sendSock);
	  return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}

	if (uciPayload->commandArgs == NULL) 
		ArgsSize = 0;
	else 
		ArgsSize = strlen(uciPayload->commandArgs);

	if ( ( bufferMessage = malloc(ArgsSize + sizeof(unsigned char) + sizeof(unsigned int)) ) != NULL ) {
		memcpy(bufferMessage, &(uciPayload->command), sizeof(unsigned char));	/* First Field */
		ArgsSizeNet = htonl(ArgsSize);
		memcpy(bufferMessage+sizeof(unsigned char), &ArgsSizeNet, sizeof(unsigned int)); /* Second Field */
		if (uciPayload->commandArgs != NULL)
			memcpy(bufferMessage+(sizeof(unsigned char)+sizeof(unsigned int)), uciPayload->commandArgs, ArgsSize); /* Third Field */	

		/*Send conf request to uci daemon */
		if ( sendto(sendSock, bufferMessage, sizeof(unsigned char), 0, (struct sockaddr *) &serv_addr, slen) < 0 ) {
			CWLog("[CWSaveUCIValues]: Error on sendto function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}
		if ( recvfrom(sendSock, &response, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, (socklen_t *) &slen) < 0) {
			CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}

		if (!ntohl(response)) {
			CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}
		if ( sendto(sendSock, bufferMessage+sizeof(unsigned char), sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, slen) < 0 ) {
			CWLog("[CWSaveUCIValues]: Error on sendto function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}

		if ( recvfrom(sendSock, &response, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, (socklen_t *) &slen) < 0) {
			CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}

		if (!ntohl(response)) {
			CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}
		if (ArgsSize > 0) {
			if ( sendto(sendSock, bufferMessage+sizeof(unsigned char)+sizeof(unsigned int), ArgsSize, 0, (struct sockaddr *) &serv_addr, slen) < 0 ) {
				CWLog("[CWSaveUCIValues]: Error on sendto function.");
				close(sendSock);
				CW_FREE_OBJECT(bufferMessage);
				return CWErrorRaise(CW_ERROR_GENERAL, NULL);
			}

			if ( recvfrom(sendSock, &response, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, (socklen_t *) &slen) < 0) {
				CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
				close(sendSock);
				CW_FREE_OBJECT(bufferMessage);
				return CWErrorRaise(CW_ERROR_GENERAL, NULL);
			}

			if (!ntohl(response)) {
				CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
				close(sendSock);
				CW_FREE_OBJECT(bufferMessage);
				return CWErrorRaise(CW_ERROR_GENERAL, NULL);
			}
		}
	} else {
		CWLog("[CWSaveUCIValues]: Error on malloc function.");
		close(sendSock);
		CW_FREE_OBJECT(bufferMessage);
		return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}

	/*Receive result code from uci daemon*/
	if ( recvfrom(sendSock, &responseCode, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, (socklen_t *) &slen) < 0) {
			CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}

	response = htonl(1);
	
	if ( sendto(sendSock, &response, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, slen) < 0 ) {
		CWLog("[CWSaveUCIValues]: Error on sendto function.");
		close(sendSock);
		CW_FREE_OBJECT(bufferMessage);
		return CWErrorRaise(CW_ERROR_GENERAL, NULL);
	}

	responseCode = ntohl(responseCode);

	if (responseCode != 0) 
		/*If we have an error*/
		*resultCode = CW_PROTOCOL_FAILURE;
	else {
		/*OK, that's a go... receive response string (if we have it)*/
		/*Receive response length from uci daemon*/
		if ( recvfrom(sendSock, &responseSize, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr,(socklen_t *) &slen) < 0) {
			CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}

		if ( sendto(sendSock, &response, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, slen) < 0 ) {
			CWLog("[CWSaveUCIValues]: Error on sendto function.");
			close(sendSock);
			CW_FREE_OBJECT(bufferMessage);
			return CWErrorRaise(CW_ERROR_GENERAL, NULL);
		}

		responseSize = ntohl(responseSize);

		if (responseSize > 0) {

			CW_FREE_OBJECT(bufferMessage);

			/*Fill buffer for response*/
			if ((bufferMessage = malloc(responseSize*sizeof(unsigned char))) != NULL) {

				/*Receive from uci daemon the response string*/
				if ( recvfrom(sendSock, bufferMessage, sizeof(unsigned char)*responseSize, 0, (struct sockaddr *) &serv_addr, (socklen_t *) &slen) < 0) {
					CWLog("[CWSaveUCIValues]: Error on recvfrom function.");
					close(sendSock);
					CW_FREE_OBJECT(bufferMessage);
					return CWErrorRaise(CW_ERROR_GENERAL, NULL);
				}
				if ( sendto(sendSock, &response, sizeof(unsigned int), 0, (struct sockaddr *) &serv_addr, slen) < 0 ) {
					CWLog("[CWSaveUCIValues]: Error on sendto function.");
					close(sendSock);
					CW_FREE_OBJECT(bufferMessage);
					return CWErrorRaise(CW_ERROR_GENERAL, NULL);
				}

				/*Copy the buffer in the uci payload structure*/
				CW_CREATE_STRING_ERR(uciPayload->response, responseSize, return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL););
				memcpy(uciPayload->response, bufferMessage, responseSize);
				uciPayload->response[responseSize] = '\0';

			} else {
				CWLog("[CWSaveUCIValues]: Error on malloc function.");
				close(sendSock);
				return CWErrorRaise(CW_ERROR_GENERAL, NULL);
			}
		} else {
			/*If response is empty, set it to NULL (needed after, when the response is built)*/
			uciPayload->response = NULL;
		}
	}

	CWLog("Saved UCI Vendor Payload...");
	close(sendSock);	
	CW_FREE_OBJECT(bufferMessage);
	return CW_TRUE;		
}
