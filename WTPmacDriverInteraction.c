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



/*prototipi da includere in CWWTP.h*/
int set_rts_threshold(int value);
int get_rts_threshold(int* value);

int set_frag_threshold(int value);
int get_frag_threshold(int *value);


/*--------------------------- RTS/CTS Threshold ---------------------------*/
int set_rts_threshold(int value){
	
	printf("\nRTS/CTS threshold impostato a: %d\n",value);
	return 1;
}

int get_rts_threshold(int *value){

	printf("\nRTS/CTS threshold: %d\n", *value);
	return 1;
}

/*--------------------------- Fragmentation Threshold ---------------------------*/
int set_frag_threshold(int value){

	printf("\nFragmentation threshold impostato a: %d\n",value);
	return 1;
}

int get_frag_threshold(int *value){

	printf("\nFragmentation threshold: %d\n", *value);
	return 1;
}


int set_txq(int code, int cwmin, int cwmax, int aifs, int burst_time){
	char str[32];
	sprintf(str,"X%d %d %d %d %d", code, cwmin, cwmax, aifs, burst_time);
	
	CWWTPsend_command_to_hostapd_SET_TXQ(str, strlen(str));
	return 1;
}

/*set CWMIN*/
int set_wme_cwmin(int class,int value){

	printf("\nCWMIN impostato a: %d\n", value);
	return 1;
}

/*set CWMAX*/
int set_wme_cwmax(int class,int value){

	printf("\nCWMAX impostato a: %d\n", value);
	return 1;
}

/*set AIFSN*/
int set_wme_aifsn(int class,int value){

	printf("\nAIFSN impostato a: %d\n", value);
	return 1;
}


