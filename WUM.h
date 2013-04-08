/*
 * WUM.h - WTP Update Messages
 *
 *
 * 4/11/2009 - Donato Capitella (d.capitella@gmail.com)
 */

#ifndef WUM_H
#define WUM_H

#include "CWCommon.h"

#define WTP_VERSION_MAJOR	0
#define WTP_VERSION_MINOR	93
#define WTP_VERSION_REVISION	2


#define WTP_VERSION_REQUEST	1
#define WTP_VERSION_RESPONSE    2

#define WTP_UPDATE_REQUEST     	3
#define WTP_UPDATE_RESPONSE    	4

#define WTP_CUP_FRAGMENT     	5
#define WTP_CUP_ACK     	6

#define WTP_COMMIT_UPDATE	7
#define WTP_COMMIT_ACK          8

#define WTP_CANCEL_UPDATE_REQUEST	9
#define WTP_CANCEL_UPDATE_RESPONSE	10

#define WTP_LOCK_FILE           "wtp.lock"

void WTPUpdateAgent(char *CupPath);

#endif /* WUM_H */
