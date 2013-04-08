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

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#define SIOCIWFIRSTPRIV			0x8BE0
#define IEEE80211_IOCTL_SETWMMPARAMS 	(SIOCIWFIRSTPRIV+4)
#define IEEE80211_IOCTL_GETWMMPARAMS 	(SIOCIWFIRSTPRIV+5)
#define IEEE80211_WMMPARAMS_CWMIN	1
#define IEEE80211_WMMPARAMS_CWMAX	2
#define IEEE80211_WMMPARAMS_AIFS	3

/**************************** mac80211 ****************************/


/**************************** iwconfig ****************************/
/*--------------------------- Frequency ---------------------------*/
int set_freq(int sock, struct iwreq wrq, int value)
{
	wrq.u.freq.m=value;		//in Ghz/10
	wrq.u.freq.e=1;		
	
      	if(ioctl(sock, SIOCSIWFREQ, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nFrequenza impostata a: %d\n", wrq.u.freq.m);

	return 1;
}

int get_freq(int sock, struct iwreq* wrq)
{
      	if(ioctl(sock, SIOCGIWFREQ, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nFrequenza: %d\n", wrq->u.freq.m);

	return 1;
}

/*--------------------------- Bit rate ---------------------------*/
int set_bitrate(int sock, struct iwreq wrq, int value)
{
	wrq.u.bitrate.value=value;
	wrq.u.bitrate.fixed=1;
	
      	if(ioctl(sock, SIOCSIWRATE, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nBit rate impostato a: %d\n", wrq.u.bitrate.value);

	return 1;
}

int get_bitrate(int sock, struct iwreq* wrq)
{
      	if(ioctl(sock, SIOCGIWRATE, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nBit rate: %d\n", wrq->u.bitrate.value);

	return 1;
}

/*--------------------------- RTS/CTS Threshold ---------------------------*/
int set_rts_cts(int sock, struct iwreq wrq, int value)
{
	if (value!=0) {wrq.u.rts.value=value;}
	else {wrq.u.rts.disabled=1;}	

      	if(ioctl(sock, SIOCSIWRTS, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nRTS/CTS threshold impostato a: %d\n", wrq.u.rts.value);

	return 1;
}

int get_rts_cts(int sock, struct iwreq* wrq)
{
      	if(ioctl(sock, SIOCGIWRTS, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	if (wrq->u.rts.disabled!=1) {printf("\nRTS/CTS threshold: %d\n", wrq->u.rts.value);}
	else {printf("\nRTS/CTS threshold off\n");}

	return 1;
}

/*--------------------------- Fragmentation Threshold ---------------------------*/
int set_frag(int sock, struct iwreq wrq, int value)
{
	if (value!=0) {wrq.u.frag.value=value;}
	else {wrq.u.frag.disabled=1;}	

      	if(ioctl(sock, SIOCSIWFRAG, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nFragmentation threshold impostato a: %d\n", wrq.u.frag.value);

	return 1;
}

int get_frag(int sock, struct iwreq* wrq)
{
      	if(ioctl(sock, SIOCGIWFRAG, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	if (wrq->u.frag.disabled!=1) {printf("\nFragmentation threshold: %d\n", wrq->u.frag.value);}
	else {printf("\nFragmentation threshold off\n");}

	return 1;
}

/*--------------------------- Transmit Power ---------------------------*/
int set_txpower(int sock, struct iwreq wrq, int value)
{
	wrq.u.txpower.value=value; 
	wrq.u.txpower.fixed=1;

      	if(ioctl(sock, SIOCSIWTXPOW, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nTransmit power impostato a: %d\n", wrq.u.txpower.value);

	return 1;
}

int get_txpower(int sock, struct iwreq* wrq)
{
      	if(ioctl(sock, SIOCGIWTXPOW, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	if (wrq->u.txpower.disabled!=1) {printf("\nTransmit power: %d\n", wrq->u.txpower.value);}
	else {printf("\nTransmit power off\n");}

	return 1;
}

/**************************** iwpriv ****************************/
/*--------------------------- CWMIN ---------------------------*/
int set_cwmin(int sock, struct iwreq wrq, int acclass, int sta, int value)
{
	int buffer[3];

	wrq.u.mode=IEEE80211_WMMPARAMS_CWMIN;
	buffer[0]=acclass;
	buffer[1]=sta;
	buffer[2]=value;
	memcpy(wrq.u.name + sizeof(int), buffer, sizeof(buffer));

      	if(ioctl(sock, IEEE80211_IOCTL_SETWMMPARAMS, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	

	printf("\nCWMIN impostato a: %d\n", value);

	return 1;
}

int get_cwmin(int sock, struct iwreq* wrq, int acclass, int sta)
{
	int buffer[2];

	wrq->u.mode=IEEE80211_WMMPARAMS_CWMIN;
	buffer[0]=acclass;
	buffer[1]=sta;
	memcpy(wrq->u.name + sizeof(int), buffer, sizeof(buffer));

    	if(ioctl(sock, IEEE80211_IOCTL_GETWMMPARAMS, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
//	printf("\nCWMIN: %d\n", wrq->u.param.value);

	return 1;
}

/*--------------------------- CWMAX ---------------------------*/
int set_cwmax(int sock, struct iwreq wrq, int acclass, int sta, int value)
{
	int buffer[3];

	wrq.u.mode=IEEE80211_WMMPARAMS_CWMAX;
	buffer[0]=acclass;
	buffer[1]=sta;
	buffer[2]=value;
	memcpy(wrq.u.name + sizeof(int), buffer, sizeof(buffer));

      	if(ioctl(sock, IEEE80211_IOCTL_SETWMMPARAMS, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nCWMAX impostato a: %d\n", value);

	return 1;
}

int get_cwmax(int sock, struct iwreq* wrq, int acclass, int sta)
{
	int buffer[2];

	wrq->u.mode=IEEE80211_WMMPARAMS_CWMAX;
	buffer[0]=acclass;
	buffer[1]=sta;
	memcpy(wrq->u.name + sizeof(int), buffer, sizeof(buffer));

      	if(ioctl(sock, IEEE80211_IOCTL_GETWMMPARAMS, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	//printf("\nCWMAX: %d\n", wrq->u.param.value);

	return 1;
}

/*--------------------------- AIFS ---------------------------*/
int set_aifs(int sock, struct iwreq wrq, int acclass, int sta, int value)
{
	int buffer[3];

	wrq.u.mode=IEEE80211_WMMPARAMS_AIFS;
	buffer[0]=acclass;
	buffer[1]=sta;
	buffer[2]=value;
	memcpy(wrq.u.name + sizeof(int), buffer, sizeof(buffer));

      	if(ioctl(sock, IEEE80211_IOCTL_SETWMMPARAMS, &wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	printf("\nAIFS impostato a: %d\n", value);

	return 1;
}

int get_aifs(int sock, struct iwreq* wrq, int acclass, int sta)
{
	int buffer[2];

	wrq->u.mode=IEEE80211_WMMPARAMS_AIFS;
	buffer[0]=acclass;
	buffer[1]=sta;
	memcpy(wrq->u.name + sizeof(int), buffer, sizeof(buffer));

     	if(ioctl(sock, IEEE80211_IOCTL_GETWMMPARAMS, wrq) < 0)
	{
		perror("Ioctl error");
		return(0);
	}	
	
	//printf("\nAIFS: %d\n", wrq->u.param.value);

	return 1;
}
