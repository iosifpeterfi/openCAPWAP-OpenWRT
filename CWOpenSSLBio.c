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

static int memory_write(BIO *h, const char *buf, int num);
static int memory_read(BIO *h, char *buf, int size);
static int memory_puts(BIO *h, const char *str);
static long memory_ctrl(BIO *h, int cmd, long arg1, void *arg2);
static int memory_new(BIO *h);
static int memory_free(BIO *data);

#ifndef IP_MTU
#define IP_MTU		14
#endif

typedef struct
{
	CWSocket sock;
	CWNetworkLev4Address sendAddress;
	CWSafeList* pRecvAddress;
	unsigned int nMtu;
} BIO_memory_data;

static BIO_METHOD methods_memory = {
	BIO_TYPE_DGRAM,
	"memory packet",
	memory_write,
	memory_read,
	memory_puts,
	NULL, /* dgram_gets, */
	memory_ctrl,
	memory_new,
	memory_free,
	NULL,
};

BIO_METHOD* BIO_s_memory(void)
{
	return(&methods_memory);
}

BIO* BIO_new_memory(CWSocket sock, CWNetworkLev4Address* pSendAddress, CWSafeList* pRecvAddress)
{
	BIO *ret;
	BIO_memory_data* pData;

	ret = BIO_new(BIO_s_memory());
	if (ret == NULL) 
		return NULL;

	//
	pData = (BIO_memory_data*)ret->ptr;
	pData->sock = sock;
	memcpy(&pData->sendAddress, pSendAddress, sizeof(CWNetworkLev4Address));
	pData->pRecvAddress = pRecvAddress;

	return ret;
}

static int memory_new(BIO *bi)
{
	bi->init = 1;
	bi->num = 0;
	bi->flags = 0;
	bi->ptr = (char*)malloc(sizeof(BIO_memory_data));

	return 1;
}

static int memory_free(BIO *a)
{
	if (a == NULL) 
		return 0;

	free(a->ptr);
	return 1;
}

static int memory_read(BIO *b, char *out, int outl)
{
	int ret = -1;
	char* buf;
	int size;
	BIO_memory_data* pData = (BIO_memory_data*)b->ptr;

	//
	//BIO_clear_retry_flags(b);

	//
	CWLockSafeList(pData->pRecvAddress);

	// Used only in DTLS handshake
	while (CWGetCountElementFromSafeList(pData->pRecvAddress) == 0){
		CWWaitElementFromSafeList(pData->pRecvAddress);
	}

	buf = (char*)CWRemoveHeadElementFromSafeList(pData->pRecvAddress, &size);

	CWUnlockSafeList(pData->pRecvAddress);

	if ((buf == NULL) || (size <= 0))
		CWLog("Warning empty buffer");
	else
	{
		ret = ((size < outl) ? size : outl) - 4;	
		memcpy(out, buf + 4, ret);
		CW_FREE_OBJECT(buf);
	}

	return ret;
}

static int memory_write(BIO *b, const char *in, int inl)
{
	int ret = -1;
	char strBuffer[MAX_UDP_PACKET_SIZE];
	BIO_memory_data* pData = (BIO_memory_data*)b->ptr;
	
	//
	strBuffer[0] = (char)(CW_PROTOCOL_VERSION << 4) | (char)(CW_PACKET_CRYPT);
	strBuffer[1] = strBuffer[2] = strBuffer[3] = 0;

	//
	memcpy(&strBuffer[4], in, inl);

	//
	errno = 0;
	ret = sendto(pData->sock, strBuffer, inl + 4, 0, (struct sockaddr*)&pData->sendAddress, sizeof(struct sockaddr_storage));

	//BIO_clear_retry_flags(b);
	if (ret <= 0)
	{
		if (errno == EINTR)
			BIO_set_retry_write(b);  
	}
	else
	{
		ret -= 4;
	}
	
	return ret;
}

static long memory_ctrl(BIO *b, int cmd, long num, void *ptr)
{
	long ret = 1;
	long sockopt_val = 0;
	unsigned int sockopt_len = 0;
	BIO_memory_data* pData = (BIO_memory_data*)b->ptr;

	switch (cmd)
	{
		case BIO_CTRL_RESET:
			ret = 0;
			break;
		
		case BIO_CTRL_EOF:
			ret = 0;
			break;

		case BIO_CTRL_INFO:
			ret = 0;
			break;

		case BIO_CTRL_GET_CLOSE:
			ret = 0;
			break;

		case BIO_CTRL_SET_CLOSE:
			break;

		case BIO_CTRL_WPENDING:
			ret = 0;
			break;
	
		case BIO_CTRL_PENDING:
			ret = 0;
			break;

		case BIO_CTRL_DUP:
			ret = 1;
			break;

		case BIO_CTRL_FLUSH:
			ret = 1;
			break;

		case BIO_CTRL_PUSH:
			ret = 0;
			break;

		case BIO_CTRL_POP:
			ret = 0;

		case BIO_CTRL_DGRAM_QUERY_MTU:
		{
         	sockopt_len = sizeof(sockopt_val);
			if ((ret = getsockopt(pData->sock, IPPROTO_IP, IP_MTU, (void *)&sockopt_val, &sockopt_len)) < 0 || sockopt_val < 0)
			{ 
				ret = 0; 
			}
			else
			{
				pData->nMtu = sockopt_val;
				ret = sockopt_val;
			}

			break;	
		}

		case BIO_CTRL_DGRAM_GET_MTU:
			ret = pData->nMtu;
			break;
	
		case BIO_CTRL_DGRAM_SET_MTU:
			pData->nMtu = num;
			ret = num;
			break;

		default:
			ret = 0;
			break;
	}

	return ret;
}

static int memory_puts(BIO *bp, const char *str)
{
	return memory_write(bp, str, strlen(str));
}
