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

#include "CWWTP.h"
#include "WTPBcmDriverInteraction.h"



static char interface[16] = "wl0";
static char wlbuf[WLC_IOCTL_MAXLEN];
#define BUFSIZE 8192

/*prototipi da includere in CWWTP.h*/
int set_rts_threshold(int value);
int get_rts_threshold(int* value);

int set_frag_threshold(int value);
int get_frag_threshold(int *value);

int set_maxassoc(int value);
int get_maxassoc(int *value);



/*--------------------------- RTS/CTS Threshold ---------------------------*/
int set_rts_threshold(int value)
{
	char *iov_type;

	iov_type=malloc(20*sizeof(char));
	strcpy(iov_type,"rtsthresh");
	
	 if(wl_iovar_setint(interface, iov_type, value) < 0){
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nRTS/CTS threshold impostato a: %d\n",value);
	return 1;
}

int get_rts_threshold(int *value)
{
	char *iov_type;

	iov_type=malloc(20*sizeof(char));
	strcpy(iov_type,"rtsthresh");
	
	 if(wl_iovar_getint(interface, iov_type, value) < 0){
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nRTS/CTS threshold: %d\n", *value);
	return 1;
}

/*--------------------------- Fragmentation Threshold ---------------------------*/
int set_frag_threshold(int value)
{
	char *iov_type;

	iov_type=malloc(20*sizeof(char));
	strcpy(iov_type,"fragthresh");
	
	 if(wl_iovar_setint(interface, iov_type, value) < 0){
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nFragmentation threshold impostato a: %d\n",value);
	return 1;
}

int get_frag_threshold(int *value)
{
	char *iov_type;

	iov_type=malloc(20*sizeof(char));
	strcpy(iov_type,"fragthresh");
	
	 if(wl_iovar_getint(interface, iov_type, value) < 0){
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nFragmentation threshold: %d\n", *value);
	return 1;
}


/*--------------------------- Max. number of associated clients ---------------------------*/
int set_maxassoc(int value)
{
	char *iov_type;

	iov_type=malloc(20*sizeof(char));
	strcpy(iov_type,"maxassoc");
	
	 if(wl_iovar_setint(interface, iov_type, value) < 0){
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nMax. number of associated clients impostato a: %d\n",value);
	return 1;
}

int get_maxassoc(int *value)
{
	char *iov_type;

	iov_type=malloc(20*sizeof(char));
	strcpy(iov_type,"maxassoc");
	
	 if(wl_iovar_getint(interface, iov_type, value) < 0){
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nMax. number of associated clients: %d\n", *value);
	return 1;
}

/*---------- WME Extensions: cwmin,cwmax,aifsn for BE,BK,VI,VO queues ---------------------*/
            /*the Broadcom's driver doesn't support GET command for wme.*/

/*set CWMIN*/
int set_wme_cwmin(int class,int value)
{
	edcf_acparam_t params[AC_COUNT];
	char *buf = wlbuf;
	char *type;

	type=malloc(20*sizeof(char));
	strcpy(type,"wme_ac_ap");

	memset(params, 0, sizeof(params));
	wl_iovar_get(interface, type, params, sizeof(params));

	strcpy(buf, type);
	buf += strlen(buf) + 1;

	params[class].ECW = (params[class].ECW & ~(0xf)) | (value & 0xf);
	memcpy(buf, &params[class], sizeof(edcf_acparam_t));

	if( wl_ioctl(interface, WLC_SET_VAR, wlbuf, BUFSIZE) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	

	printf("\nCWMIN impostato a: %d\n", value);
	return 1;
}

/*set CWMAX*/
int set_wme_cwmax(int class,int value)
{
	edcf_acparam_t params[AC_COUNT];
	char *buf = wlbuf;
	char *type;

	type=malloc(20*sizeof(char));
	strcpy(type,"wme_ac_ap");

	memset(params, 0, sizeof(params));
	wl_iovar_get(interface, type, params, sizeof(params));

	strcpy(buf, type);
	buf += strlen(buf) + 1;

	params[class].ECW = (params[class].ECW & ~(0xf << 4)) | ((value & 0xf) << 4);
	memcpy(buf, &params[class], sizeof(edcf_acparam_t));

	if( wl_ioctl(interface, WLC_SET_VAR, wlbuf, BUFSIZE) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	

	printf("\nCWMAX impostato a: %d\n", value);
	return 1;
}

/*set AIFSN*/
int set_wme_aifsn(int class,int value)
{
	edcf_acparam_t params[AC_COUNT];
	char *buf = wlbuf;
	char *type;

	type=malloc(20*sizeof(char));
	strcpy(type,"wme_ac_ap");

	memset(params, 0, sizeof(params));
	wl_iovar_get(interface, type, params, sizeof(params));

	strcpy(buf, type);
	buf += strlen(buf) + 1;

	params[class].ACI = (params[class].ACI & ~(0xf)) | (value & 0xf);
	memcpy(buf, &params[class], sizeof(edcf_acparam_t));

	if( wl_ioctl(interface, WLC_SET_VAR, wlbuf, BUFSIZE) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	

	printf("\nAIFSN impostato a: %d\n", value);
	return 1;
}



/*@@@@@@@@ ioctl driver interaction's functions @@@@@@@@@@@@@*/

int
wl_ioctl(char *name, int cmd, void *buf, int len)
{
	struct ifreq ifr;
	wl_ioctl_t ioc;
	int ret = 0;
	int s;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		return errno;
	}

	/* do it */
	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	ifr.ifr_data = (caddr_t) &ioc;
	if ((ret = ioctl(s, SIOCDEVPRIVATE, &ifr)) < 0) {
		perror("ioctl error");
		close(s);
		return -1;
	}
		
	/* cleanup */	
	close(s);
	return ret;
}

static int
wl_iovar_getbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen)
{
	int err;
	uint namelen;
	uint iolen;

	namelen = strlen(iovar) + 1;	 /* length of iovar name plus null */
	iolen = namelen + paramlen;

	/* check for overflow */
	if (iolen > buflen)
		return (BCME_BUFTOOSHORT);

	memcpy(bufptr, iovar, namelen);	/* copy iovar name including null */
	memcpy((int8*)bufptr + namelen, param, paramlen);

	err = wl_ioctl(ifname, WLC_GET_VAR, bufptr, buflen);

	return (err);
}

static int
wl_iovar_setbuf(char *ifname, char *iovar, void *param, int paramlen, void *bufptr, int buflen)
{
	uint namelen;
	uint iolen;

	namelen = strlen(iovar) + 1;	 /* length of iovar name plus null */
	iolen = namelen + paramlen;

	/* check for overflow */
	if (iolen > buflen)
		return (BCME_BUFTOOSHORT);

	memcpy(bufptr, iovar, namelen);	/* copy iovar name including null */
	memcpy((int8*)bufptr + namelen, param, paramlen);

	return wl_ioctl(ifname, WLC_SET_VAR, bufptr, iolen);
}

int
wl_iovar_set(char *ifname, char *iovar, void *param, int paramlen)
{
	char smbuf[WLC_IOCTL_SMLEN];

	return wl_iovar_setbuf(ifname, iovar, param, paramlen, smbuf, sizeof(smbuf));
}

int
wl_iovar_get(char *ifname, char *iovar, void *bufptr, int buflen)
{
	char smbuf[WLC_IOCTL_SMLEN];
	int ret;

	/* use the return buffer if it is bigger than what we have on the stack */
	if (buflen > sizeof(smbuf)) {
		ret = wl_iovar_getbuf(ifname, iovar, NULL, 0, bufptr, buflen);
	} else {
		ret = wl_iovar_getbuf(ifname, iovar, NULL, 0, smbuf, sizeof(smbuf));
		if (ret == 0)
			memcpy(bufptr, smbuf, buflen);
	}

	return ret;
}
