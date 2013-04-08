/*
 * Linux ioctl helper functions for driver wrappers
 * Copyright (c) 2002-2010, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */


#include "utils/includes.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>

#include "utils/common.h"
#include "linux_ioctl_fake.h"



int linux_set_iface_flags(int sock, const char *ifname, int dev_up){
	
	return 0;
}

int linux_iface_up(int sock, const char *ifname){

	return 0;
}

int linux_get_ifhwaddr(int sock, const char *ifname, u8 *addr){

	int i=2;
	
	if(i==0){ // TP-LINK (Atheros)
		addr[0]=0xB0;
		addr[1]=0x48;
		addr[2]=0x7A;
		addr[3]=0x93;
		addr[4]=0x90;
		addr[5]=0xF8;
		
	}else if(i==1){// D-Link 
		addr[0]=0x28;
		addr[1]=0x10;
		addr[2]=0x7B;
		addr[3]=0x44;
		addr[4]=0xd0;
		addr[5]=0xB1;
		
	}else if(i==2){// TP-LINK (Atheros)
		addr[0]=0xAA;
		addr[1]=0xBB;
		addr[2]=0xCC;
		addr[3]=0xDD;
		addr[4]=0xEE;
		addr[5]=0xFF;
		
	}

	return 0;
}

int linux_set_ifhwaddr(int sock, const char *ifname, const u8 *addr){

	return 0;
}


#ifndef SIOCBRADDBR
#define SIOCBRADDBR 0x89a0
#endif
#ifndef SIOCBRDELBR
#define SIOCBRDELBR 0x89a1
#endif
#ifndef SIOCBRADDIF
#define SIOCBRADDIF 0x89a2
#endif
#ifndef SIOCBRDELIF
#define SIOCBRDELIF 0x89a3
#endif


int linux_br_add(int sock, const char *brname){
	return 0;
}

int linux_br_del(int sock, const char *brname){
	return 0;
}

int linux_br_add_if(int sock, const char *brname, const char *ifname){
	return 0;
}

int linux_br_del_if(int sock, const char *brname, const char *ifname){
	return 0;
}

int linux_br_get(char *brname, const char *ifname){
	return 0;
}


