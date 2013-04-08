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


#ifndef __CAPWAP_CWStevens_HEADER__
#define __CAPWAP_CWStevens_HEADER__
 
#include <net/if.h>
#include <sys/ioctl.h>

#define	IFI_NAME	16			/* same as IFNAMSIZ in <net/if.h> */
#define	IFI_HADDR	 8			/* allow for 64-bit EUI-64 in future */

struct ifi_info {
  char    ifi_name[IFI_NAME];	/* interface name, null-terminated */
  short   ifi_index;			/* interface index */
  short   ifi_flags;			/* IFF_xxx constants from <net/if.h> */
  struct sockaddr  *ifi_addr;	/* primary address */
  struct sockaddr  *ifi_brdaddr;/* broadcast address */
  struct ifi_info  *ifi_next;	/* next of these structures */
};

struct ifi_info* get_ifi_info(int, int);
void free_ifi_info(struct ifi_info *);
char *sock_ntop_r(const struct sockaddr *sa, char *str);
int sock_cpy_addr_port(struct sockaddr *sa1, const struct sockaddr *sa2);
void sock_set_port_cw(struct sockaddr *sa, int port);
int sock_cmp_port(const struct sockaddr *sa1, const struct sockaddr *sa2, socklen_t salen);
int sock_cmp_addr(const struct sockaddr *sa1, const struct sockaddr *sa2, socklen_t salen);
int mcast_join(int sockfd, const struct sockaddr *grp, socklen_t grplen, const char *ifname, u_int ifindex);
int Writen(int fd, void *ptr, size_t nbytes);

#endif	/* __CAPWAP_CWStevens_HEADER__ */
