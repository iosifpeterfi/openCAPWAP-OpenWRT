/************************************************************************************************
 * Copyright (c) 2006-2009 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica   *
 *                          Universita' Campus BioMedico - Italy                                *
 *                                                                                              *
 * This program is free software; you can redistribute it and/or modify it under the terms      *
 * of the GNU General Public License as published by the Free Software Foundation; either       *
 * version 2 of the License, or (at your option) any later version.                             *
 *                                                                                              *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY              *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A         *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.                     *
 *                                                                                              *
 * You should have received a copy of the GNU General Public License along with this            *
 * program; if not, write to the:                                                               *
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,                         *
 * MA  02111-1307, USA.                                                                         *
 *                                                                                              *
 * -------------------------------------------------------------------------------------------- *
 * Project:  Capwap
 *
 *   Authors : Antonio Davoli (antonio.davoli@gmail.com)
 *   Donato Capitella (d.capitella@gmail.com)
 ************************************************************************************************/

#include "CWAC.h"
#include "ACAppsProtocol.h"
#include "CWVendorPayloads.h"
#include "WUM.h"

#define LIST_CMD "LIST"
#define SCAN_CMD "SCAN"
#define QUIT_CMD "QUIT"

#define LISTEN_PORT 1235
#define COMMAND_BUFFER_SIZE 5120
#define WTP_LIST_BUFFER_SIZE 1024

#define CMD_TYPE_SIZE   sizeof(unsigned char)

int is_valid_wtp_index(int index);
int Readn(int sock, void *buf, size_t n);

/********************************************************************
 * Now the only parameter need by the application thread manager    *
 * is the index of socket.                                          *
 ********************************************************************/

typedef struct {
	int index;
} CWInterfaceThreadArg;

/************************************************************************
 * CWOFDMSetValues provide to set the command values (type, parameters, *
 * output socket) on the correct wtp structure.                         *
 ************************************************************************/

int CWOFDMSetValues(int selection, int socketIndex, OFDMControlValues * ofdmValues)
{

	CWThreadMutexLock(&(gWTPs[selection].interfaceMutex));

	gWTPs[selection].ofdmValues = ofdmValues;
	gWTPs[selection].interfaceCommand = OFDM_CONTROL_CMD;
	gWTPs[selection].applicationIndex = socketIndex;
	CWSignalThreadCondition(&gWTPs[selection].interfaceWait);
	CWWaitThreadCondition(&gWTPs[selection].interfaceComplete, &gWTPs[selection].interfaceMutex);

	CWThreadMutexUnlock(&(gWTPs[selection].interfaceMutex));

	return 0;
}

int CWVendorSetValues(int selection, int socketIndex, CWProtocolVendorSpecificValues * vendorValues)
{

	CWThreadMutexLock(&(gWTPs[selection].interfaceMutex));

	gWTPs[selection].vendorValues = vendorValues;
	gWTPs[selection].interfaceCommand = UCI_CONTROL_CMD;
	gWTPs[selection].applicationIndex = socketIndex;
	CWSignalThreadCondition(&gWTPs[selection].interfaceWait);
	CWWaitThreadCondition(&gWTPs[selection].interfaceComplete, &gWTPs[selection].interfaceMutex);

	CWThreadMutexUnlock(&(gWTPs[selection].interfaceMutex));

	return 0;
}

int CWWumSetValues(int selection, int socketIndex, CWProtocolVendorSpecificValues * vendorValues)
{

	CWThreadMutexLock(&(gWTPs[selection].interfaceMutex));

	gWTPs[selection].vendorValues = vendorValues;
	gWTPs[selection].interfaceCommand = WTP_UPDATE_CMD;
	gWTPs[selection].applicationIndex = socketIndex;
	CWSignalThreadCondition(&gWTPs[selection].interfaceWait);
	CWWaitThreadCondition(&gWTPs[selection].interfaceComplete, &gWTPs[selection].interfaceMutex);

	CWThreadMutexUnlock(&(gWTPs[selection].interfaceMutex));

	return 0;
}

/************************************************************************
 * CWManageApplication is the function that provide the management of   *
 * interaction with a single application.                               *
 * -------------------------------------------------------------------- *
 * The messages used are defined in ACAppsProtocol.h                    *
 ************************************************************************/

CW_THREAD_RETURN_TYPE CWManageApplication(void *arg)
{

	int socketIndex = ((CWInterfaceThreadArg *) arg)->index;
	CWSocket sock = appsManager.appSocket[socketIndex];
	int n, connected = htonl(CONNECTION_OK), gActiveWTPsTemp;

	char commandBuffer[COMMAND_BUFFER_SIZE];
	char wtpListBuffer[WTP_LIST_BUFFER_SIZE];

	int payload_size;
	int i, nameLength, numActiveWTPs = 0, wtpIndex;
	int iTosend, nLtoSend;
	unsigned char cmd_msg, msg_elem;
	OFDMControlValues *ofdmValues;
	CWProtocolVendorSpecificValues *vendorValues;
	CWVendorUciValues *uciValues;
	CWVendorWumValues *wumValues;

	/********************************************************************************
	 * Write on application socket that connection setting is happened correctly.   *
	 ********************************************************************************/

	if (Writen(sock, &connected, sizeof(int)) < 0) {
		CWLog("Error on writing on application socket ");
		return NULL;
	}

	/*
	 * Before starting, make sure to detach the thread because the parent
	 * doesn't do a join and we risk resource leaks.
	 *
	 * ref -> BUG-TRL01
	 * 15/10/2009 - Donato Capitella
	 */

	pthread_detach(pthread_self());	// no need here to check return value

	/************************
	 *   Thread Main Loop   *
	 ************************/

	CW_REPEAT_FOREVER {

		memset(commandBuffer, 0, COMMAND_BUFFER_SIZE);

		/****************************************
		 *      Waiting for client commands     *
		 ****************************************/

		if ((n = Readn(sock, &cmd_msg, CMD_TYPE_SIZE)) > 0) {

			if (cmd_msg == QUIT_MSG) {
				/****************************************************
				 *      Quit Message: Clean socket information      *
				 ****************************************************/
				goto quit_manage;
			}

			if (cmd_msg == LIST_MSG) {

				/****************************************
				 *          Manage LIST command         *
				 * ------------------------------------ *
				 * 1. Get Number of active WTPs,        *
				 * 2. Create Message Answer,            *
				 * 3. Send To client socket.            *
				 ****************************************/

				if (!CWErr(CWThreadMutexLock(&gActiveWTPsMutex))) {
					CWLog("Error locking the mutex");
					return NULL;
				}

				gActiveWTPsTemp = gActiveWTPs;

				CWThreadMutexUnlock(&gActiveWTPsMutex);

				if (gActiveWTPsTemp > 0)
					for (i = 0; i < gMaxWTPs; i++)
						if (gWTPs[i].isNotFree)
							if (!gWTPs[i].WTPProtocolManager.name)
								gActiveWTPsTemp -= 1;

				numActiveWTPs = htonl(gActiveWTPsTemp);

				memset(wtpListBuffer, 0, sizeof(wtpListBuffer));
				payload_size = 0;

				memcpy(wtpListBuffer, &numActiveWTPs, sizeof(int));
				payload_size = sizeof(int);

				if (gActiveWTPsTemp > 0) {
					for (i = 0; i < gMaxWTPs; i++) {

						if (gWTPs[i].isNotFree) {

							if (!gWTPs[i].WTPProtocolManager.name)
								continue;

							nameLength = strlen(gWTPs[i].WTPProtocolManager.name);

							iTosend = htonl(i);
							nLtoSend = htonl(nameLength);

							memcpy(wtpListBuffer + payload_size, &iTosend, sizeof(int));
							payload_size += sizeof(int);

							memcpy(wtpListBuffer + payload_size, &nLtoSend, sizeof(int));
							payload_size += sizeof(int);

							memcpy(wtpListBuffer + payload_size,
							       gWTPs[i].WTPProtocolManager.name,
							       strlen(gWTPs[i].WTPProtocolManager.name));
							payload_size += strlen(gWTPs[i].WTPProtocolManager.name);
						}
					}
				}

				if (Writen(sock, wtpListBuffer, payload_size) < 0)
					CWLog("Error on write message on application socket");

			}

			if (cmd_msg == CONF_UPDATE_MSG) {

				/****************************************
				 * Manage CONF command          *
				 * ------------------------------------ *
				 * 1. Select the type of CONF_UPDATE,   *
				 * 2. Get Index of WTP,         *
				 * 3. Manage request.           *
				 ****************************************/

				if ((n = Readn(sock, commandBuffer, sizeof(msg_elem) + sizeof(wtpIndex))) < 0) {
					CWLog("Error while reading from socket.");
					goto quit_manage;
				}

				memcpy(&msg_elem, commandBuffer, sizeof(unsigned char));	/* CONF_UPDATE type */
				memcpy(&wtpIndex, commandBuffer + sizeof(unsigned char), sizeof(int));	/* WTP Index */

				wtpIndex = ntohl(wtpIndex);

				/* Check if WTP Index is valid */
				if (!is_valid_wtp_index(wtpIndex)) {
					CWLog("WTP Index non valid!");
					goto quit_manage;
				}

				switch (msg_elem) {
				case MSG_ELEMENT_TYPE_OFDM:{
						/* Antonio Case */
						CW_CREATE_OBJECT_ERR(ofdmValues, OFDMControlValues, {
								     CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
								     return 0;
								     }
						);
						if ((n = Readn(sock, ofdmValues, sizeof(OFDMControlValues))) < 0) {
							CWLog("Error while reading from socket");
							goto quit_manage;
						}

					/****************************************************
					 * Two behaviors availables:                        *
					 *    - One message element For All WTPs Active     *
					 *    - One message for a specific WTP              *
					 ****************************************************/

						if (wtpIndex == ALL_ACTIVE_WTPS) {	/* All wpts case */

							if (!CWErr(CWThreadMutexLock(&gActiveWTPsMutex))) {
								CWLog("Error locking the mutex");
								return NULL;
							}
							numActiveWTPs = gActiveWTPs;
							CWThreadMutexUnlock(&gActiveWTPsMutex);

							if (numActiveWTPs > 0) {
								for (i = 0; i < gMaxWTPs; i++) {
									if (gWTPs[i].isNotFree) {
										CWOFDMSetValues(i, socketIndex,
												ofdmValues);
									}
								}
							}
						} else	/* One specific Wtp Case */
							CWOFDMSetValues(wtpIndex, socketIndex, ofdmValues);

						CW_FREE_OBJECT(ofdmValues);
						break;
					}
				case MSG_ELEMENT_TYPE_VENDOR_UCI:{
						/* Matteo Case */
						/*Do stuff to parse uci payload */
						int commandLength = 0;
						CW_CREATE_OBJECT_ERR(vendorValues, CWProtocolVendorSpecificValues, {
								     CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
								     return 0;
								     }
						);
						CW_CREATE_OBJECT_ERR(uciValues, CWVendorUciValues, {
								     CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
								     return 0;
								     }
						);
						vendorValues->vendorPayloadType =
						    CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_UCI;
						uciValues->response = NULL;

						if ((n =
						     Readn(sock, commandBuffer,
							   sizeof(unsigned char) + sizeof(int))) < 0) {
							CWLog("Error while reading from socket.");
							goto quit_manage;
						}

						memcpy(&(uciValues->command), commandBuffer, sizeof(unsigned char));
						memcpy(&commandLength, commandBuffer + sizeof(unsigned char),
						       sizeof(int));
						commandLength = ntohl(commandLength);

						if (commandLength > 0) {
							if ((n =
							     Readn(sock, commandBuffer + sizeof(char) + sizeof(int),
								   commandLength)) < 0) {
								CWLog("Error while reading from socket.");
								goto quit_manage;
							}

							CW_CREATE_STRING_ERR(uciValues->commandArgs, commandLength, {
									     CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
									     return 0;
									     }
							);
							memcpy(uciValues->commandArgs,
							       commandBuffer + sizeof(unsigned char) + sizeof(int),
							       sizeof(char) * commandLength);
							uciValues->commandArgs[commandLength] = '\0';
						} else
							uciValues->commandArgs = NULL;

						vendorValues->payload = uciValues;

					/****************************************************
					 * Two behaviors availables:                        *
					 *      - One message element For All WTPs Active   *
					 *      - One message for a specific WTP            *
					 ****************************************************/

						if (wtpIndex == ALL_ACTIVE_WTPS) {	/* All wpts case */

							if (!CWErr(CWThreadMutexLock(&gActiveWTPsMutex))) {
								CWLog("Error locking the mutex");
								return NULL;
							}
							numActiveWTPs = gActiveWTPs;
							CWThreadMutexUnlock(&gActiveWTPsMutex);

							if (numActiveWTPs > 0) {
								for (i = 0; i < gMaxWTPs; i++) {
									if (gWTPs[i].isNotFree) {
										printf
										    ("Sending UCI Configuration Message to: %d - %s \n",
										     i,
										     gWTPs[i].WTPProtocolManager.name);
										CWVendorSetValues(i, socketIndex,
												  vendorValues);
									}
								}
							}
						} else	/* One specific Wtp Case */
							CWVendorSetValues(wtpIndex, socketIndex, vendorValues);
						break;
					}
				case MSG_ELEMENT_TYPE_VENDOR_WUM:{
						/* Donato's Case */

						CW_CREATE_OBJECT_ERR(vendorValues, CWProtocolVendorSpecificValues, {
								     CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
								     return 0;
								     }
						);
						CW_CREATE_OBJECT_ERR(wumValues, CWVendorWumValues, {
								     CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
								     return 0;
								     }
						);
						vendorValues->vendorPayloadType =
						    CW_MSG_ELEMENT_VENDOR_SPEC_PAYLOAD_WUM;

						/*
						 * Read WTP Update Message fields into the wumValues structure
						 */

						if ((n = Readn(sock, &(wumValues->type), sizeof(unsigned char))) < 0) {
							CWLog("Error while reading from socket.");
							goto quit_manage;
						}

						if (wumValues->type == WTP_UPDATE_REQUEST) {
							if ((n =
							     Readn(sock, commandBuffer,
								   3 * sizeof(unsigned char) + sizeof(int))) < 0) {
								CWLog("Error while reading from socket.");
								goto quit_manage;
							}

							memcpy(&(wumValues->_major_v_), commandBuffer,
							       sizeof(unsigned char));
							memcpy(&(wumValues->_minor_v_),
							       commandBuffer + sizeof(unsigned char),
							       sizeof(unsigned char));
							memcpy(&(wumValues->_revision_v_),
							       commandBuffer + 2 * sizeof(unsigned char),
							       sizeof(unsigned char));
							memcpy(&(wumValues->_pack_size_),
							       commandBuffer + 3 * sizeof(unsigned char),
							       sizeof(unsigned int));
							wumValues->_pack_size_ = ntohl(wumValues->_pack_size_);
						} else if (wumValues->type == WTP_CUP_FRAGMENT) {
							int seqNum;
							int fragSize;

							/* Read sequence number and fragment size */
							if ((n = Readn(sock, commandBuffer, 2 * sizeof(int))) < 0) {
								CWLog("Error while reading from socket.");
								goto quit_manage;
							}

							memcpy(&seqNum, commandBuffer, sizeof(int));
							memcpy(&fragSize, commandBuffer + sizeof(int), sizeof(int));

							seqNum = ntohl(seqNum);
							fragSize = ntohl(fragSize);

							if (seqNum < 0) {
								CWLog("Update with sequence number < 0 not valid.");
								goto quit_manage;
							}

							if (fragSize <= 0) {
								CWLog("Update with fragment size <= 0 not valid.");
								goto quit_manage;
							}

							/* Read update package into buffer */
							unsigned char *buf = malloc(fragSize);
							if (buf == NULL) {
								CWLog("Can't allocate memory");
								goto quit_manage;
							}

							if ((n = Readn(sock, buf, fragSize)) < 0) {
								CWLog("Error while reading from socket.");
								goto quit_manage;
							}

							wumValues->_cup_ = buf;
							wumValues->_seq_num_ = seqNum;
							wumValues->_cup_fragment_size_ = fragSize;
						}

						vendorValues->payload = wumValues;

						/* Send Request */
						if (wtpIndex == ALL_ACTIVE_WTPS) {	/* All wpts case */

							if (!CWErr(CWThreadMutexLock(&gActiveWTPsMutex))) {
								CWLog("Error locking the mutex");
								return NULL;
							}
							numActiveWTPs = gActiveWTPs;
							CWThreadMutexUnlock(&gActiveWTPsMutex);

							if (numActiveWTPs > 0) {
								for (i = 0; i < gMaxWTPs; i++) {
									if (gWTPs[i].isNotFree) {
										printf
										    ("Sending WUM Configuration Message to: %d - %s \n",
										     i,
										     gWTPs[i].WTPProtocolManager.name);
										CWWumSetValues(i, socketIndex,
											       vendorValues);
									}
								}
							}
						} else	/* One specific Wtp Case */
							CWWumSetValues(wtpIndex, socketIndex, vendorValues);
						break;
					}
				default:
					/* Error Case: Not correct msg_elem type */
					break;
				}
			}
		} else {
			CWLog("Error on receive application command (read socket)");
			goto quit_manage;
		}
	}

 quit_manage:

	if (!CWErr(CWThreadMutexLock(&appsManager.numSocketFreeMutex))) {
		CWLog("Error locking numSocketFree Mutex");
		return NULL;
	}

	appsManager.isFree[socketIndex] = CW_TRUE;
	appsManager.numSocketFree++;

	CWDestroyThreadMutex(&appsManager.socketMutex[socketIndex]);

	CWThreadMutexUnlock(&appsManager.numSocketFreeMutex);

	close(sock);
	return NULL;
}

/****************************************************************************
 * CWInterface is the function that provide the interaction between AC and  *
 * extern applications. Listen on main Inet familty socket and create a     *
 * CWManageApplication thread for every client connected.                   *
 ****************************************************************************/

CW_THREAD_RETURN_TYPE CWInterface(void *arg)
{

	CWSocket listen_sock, conn_sock;
	struct sockaddr_in servaddr;
	CWInterfaceThreadArg *argPtr;
	CWThread thread_id;
	int i, clientFull = htonl(FULL_CLIENT_CONNECTED), optValue = 1;

	/****************************************************
	 * Setup of Application Socket Management Structure *
	 ****************************************************/

	for (i = 0; i < MAX_APPS_CONNECTED_TO_AC; i++)
		appsManager.isFree[i] = CW_TRUE;

	appsManager.numSocketFree = MAX_APPS_CONNECTED_TO_AC;

	if (!CWErr(CWCreateThreadMutex(&appsManager.numSocketFreeMutex))) {
		CWLog("Error on mutex creation on appManager structure");
		return NULL;
	}

	/****************************************************
	 * Setup (Creation and filling) of main socket      *
	 ****************************************************/

	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		CWLog("Error on socket creation on Interface");
		return NULL;
	}

	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);	/* Not Extern: INADDR_ANY */
	servaddr.sin_port = htons(LISTEN_PORT);

	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optValue, sizeof(int)) == -1) {
		CWLog("Error on socket creation on Interface");
		return NULL;
	}

	/************************************
	 * Binding socket and Listen call   *
	 ************************************/

	if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) < 0) {
		CWLog("Error on Binding");
		return NULL;
	}

	if (listen(listen_sock, MAX_APPS_CONNECTED_TO_AC) < 0) {
		CWLog("Error on LIsTsocket creation");
		return NULL;
	}

	/********************************
	 *          Main Loop           *
	 ********************************/

	CW_REPEAT_FOREVER {
		if ((conn_sock = accept(listen_sock, (struct sockaddr *)NULL, NULL)) > 0) {

			/************************************************************************
			 * Check (with lock) the number of socket free at the moment of accept, *
			 * if this value is greater than 0 will be spawn a new Manage thread.   *
			 ************************************************************************/

			if (!CWErr(CWThreadMutexLock(&appsManager.numSocketFreeMutex))) {
				CWLog("Error locking numSocketFree Mutex");
				return NULL;
			}

			if (appsManager.numSocketFree > 0) {

				CW_CREATE_OBJECT_ERR(argPtr, CWInterfaceThreadArg, {
						     CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, NULL);
						     return 0;
						     }
				);

				/************************************
				 *  Search socket for application   *
				 ************************************/

				for (i = 0; i < MAX_APPS_CONNECTED_TO_AC; i++) {
					if (appsManager.isFree[i] == CW_TRUE) {
						argPtr->index = i;
						appsManager.isFree[i] = CW_FALSE;
						appsManager.appSocket[i] = conn_sock;
						break;
					}
				}

				appsManager.numSocketFree--;
				CWThreadMutexUnlock(&appsManager.numSocketFreeMutex);

				if (!CWErr(CWCreateThreadMutex(&appsManager.socketMutex[argPtr->index]))) {
					CWLog("Error on mutex creation on appManager structure");
					return NULL;
				}

				if (!CWErr(CWCreateThread(&thread_id, CWManageApplication, argPtr))) {
					CWLog("Error on thread CWManageApplication creation");
					appsManager.isFree[argPtr->index] = CW_TRUE;
					close(conn_sock);
					CW_FREE_OBJECT(argPtr);
					/*
					 * If we can't create the thread, we have to increment numSocketFree.
					 *
					 *   ref -> BUG-LE01
					 *   15/10/2009 - Donato Capitella
					 */
					if (!CWErr(CWThreadMutexLock(&appsManager.numSocketFreeMutex))) {
						CWLog("Error locking numSocketFree Mutex");
						return NULL;
					}
					appsManager.numSocketFree++;
					CWThreadMutexUnlock(&appsManager.numSocketFreeMutex);

				}
			} else {
				CWThreadMutexUnlock(&appsManager.numSocketFreeMutex);

				/****************************************************************
				 *  There isn't space for another client, send error answer.    *
				 ***************************************************************/

				if (Writen(conn_sock, &clientFull, sizeof(int)) < 0) {
					printf("Error on sending Error Message\n");
					close(conn_sock);
				}
			}
		} else
			CWLog("Error on accept (applications) system call");
	}

	CWDestroyThreadMutex(&appsManager.numSocketFreeMutex);
	close(listen_sock);
}

int is_valid_wtp_index(int wtpIndex)
{
	if (wtpIndex < gMaxWTPs && gWTPs[wtpIndex].isNotFree)
		return CW_TRUE;
	return CW_FALSE;
}

/*
 * Steven's readn().
 */
int readn(int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ((nread = recv(fd, ptr, nleft, 0)) < 0) {
			if (errno == EINTR)
				nread = 0;	/* and call read() again */
			else
				return (-1);
		} else if (nread == 0)
			break;	/* EOF */

		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);	/* return >= 0 */
}

int Readn(int fd, void *ptr, size_t nbytes)
{
	int n;

	if ((n = readn(fd, ptr, nbytes)) < 0) {
		CWLog("Error while reading data from socket.");
		return -1;
	}

	return n;
}
