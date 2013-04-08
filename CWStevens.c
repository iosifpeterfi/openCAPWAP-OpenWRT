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

 
#include	"CWCommon.h"
#include <sys/un.h>
#ifdef	HAVE_SOCKADDR_DL_STRUCT
#include	<net/if_dl.h>
#endif

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

// an extension to Unix Network Programming's library: a thread-safe version of sock_ntop and a couple of
// functions to manage port number in IPv4/6

/* include sock_ntop */
char * sock_ntop_r(const struct sockaddr *sa, char *str)
{
    char		portstr[8];

	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		if (inet_ntop(AF_INET, &sin->sin_addr, str, 128) == 0)
			return(NULL);
		if (ntohs(sin->sin_port) != 0) {
			snprintf(portstr, sizeof(portstr), ":%d", ntohs(sin->sin_port));
			strcat(str, portstr);
		}
		return(str);
	}
/* end sock_ntop */

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		str[0] = '[';
		if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, 128 - 1) == NULL)
			return(NULL);
		if (ntohs(sin6->sin6_port) != 0) {
			snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
			strcat(str, portstr);
			return(str);
		}
		return (str + 1);
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		struct sockaddr_un	*unp = (struct sockaddr_un *) sa;

			/* OK to have no pathname bound to the socket: happens on
			   every connect() unless client calls bind() first. */
		if (unp->sun_path[0] == 0)
			strcpy(str, "(no pathname bound)");
		else
			snprintf(str, 128, "%s", unp->sun_path);
		return(str);
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		struct sockaddr_dl	*sdl = (struct sockaddr_dl *) sa;

		if (sdl->sdl_nlen > 0)
			snprintf(str, 128, "%*s (index %d)",
					 sdl->sdl_nlen, &sdl->sdl_data[0], sdl->sdl_index);
		else
			snprintf(str, 128, "AF_LINK, index=%d", sdl->sdl_index);
		return(str);
	}
#endif
	default:
		snprintf(str, 128, "sock_ntop: unknown AF_xxx: %d",
				 sa->sa_family);
		return(str);
	}
    return (NULL);
}

int sock_cpy_addr_port(struct sockaddr *sa1, const struct sockaddr *sa2)
{
	sa1->sa_family = sa2->sa_family;

	switch (sa1->sa_family) {
	case AF_INET: {
		(memcpy( &((struct sockaddr_in *) sa1)->sin_addr,
					   &((struct sockaddr_in *) sa2)->sin_addr,
					   sizeof(struct in_addr)));
		((struct sockaddr_in *) sa1)->sin_port = ((struct sockaddr_in *) sa2)->sin_port;
		return 0;
	}

#ifdef	IPV6
	case AF_INET6: {
		(memcpy( &((struct sockaddr_in6 *) sa1)->sin6_addr,
					   &((struct sockaddr_in6 *) sa2)->sin6_addr,
					   sizeof(struct in6_addr)));
					   
		( ((struct sockaddr_in6 *) sa1)->sin6_port =
				((struct sockaddr_in6 *) sa2)->sin6_port);
		return 0;
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		(strcpy( ((struct sockaddr_un *) sa1)->sun_path,
					   ((struct sockaddr_un *) sa2)->sun_path));
		return 0;
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		return(-1);		/* no idea what to copy here ? */
	}
#endif
	}
    return (-1);
}

void sock_set_port_cw(struct sockaddr *sa, int port)
{
	switch (sa->sa_family) {
	case AF_INET: {
		struct sockaddr_in	*sin = (struct sockaddr_in *) sa;

		sin->sin_port = port;
		return;
	}

#ifdef	IPV6
	case AF_INET6: {
		struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

		sin6->sin6_port = port;
		return;
	}
#endif
	}

    return;
}

struct ifi_info* get_ifi_info(int family, int doaliases)
{
	struct ifi_info		*ifi, *ifihead, **ifipnext;
	int			sockfd, len, lastlen, flags, idx = 0;
	char			*ptr, *buf, lastname[IFNAMSIZ], *cptr, *sdlname;
	struct ifconf		ifc;
	struct ifreq		*ifr, ifrcopy;
	struct sockaddr_in	*sinptr;
	struct sockaddr_in6	*sin6ptr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = (char*)malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) >= 0) {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}
	ifihead = NULL;
	ifipnext = &ifihead;
	lastname[0] = 0;
	sdlname = NULL;

	for (ptr = buf; ptr < buf + ifc.ifc_len; ) {
		ifr = (struct ifreq *) ptr;

#ifdef	HAVE_SOCKADDR_SA_LEN
		len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
		switch (ifr->ifr_addr.sa_family) {
#ifdef	IPV6
		case AF_INET6:	
			len = sizeof(struct sockaddr_in6);
			break;
#endif
		case AF_INET:	
		default:	
			len = sizeof(struct sockaddr);
			break;
		}
#endif	/* HAVE_SOCKADDR_SA_LEN */
		ptr += sizeof(ifr->ifr_name) + len;	/* for next one in buffer */

#ifdef	HAVE_SOCKADDR_DL_STRUCT
		/* assumes that AF_LINK precedes AF_INET or AF_INET6 */
		if (ifr->ifr_addr.sa_family == AF_LINK) {
			struct sockaddr_dl *sdl = (struct sockaddr_dl *)&ifr->ifr_addr;
			sdlname = ifr->ifr_name;
			idx = sdl->sdl_index;
			haddr = sdl->sdl_data + sdl->sdl_nlen;
			hlen = sdl->sdl_alen;
		}
#endif

		if (ifr->ifr_addr.sa_family != family)
			continue;	/* ignore if not desired address family */

		if ( (cptr = strchr(ifr->ifr_name, ':')) != NULL)
			*cptr = 0;		/* replace colon with null */
		if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0) {
			if (doaliases == 0)
				continue;	/* already processed this interface */
		}
		memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

		ifrcopy = *ifr;
		ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
		flags = ifrcopy.ifr_flags;
		if ((flags & IFF_UP) == 0)
			continue;	/* ignore if interface not up */

		ifi = (struct ifi_info*)calloc(1, sizeof(struct ifi_info));
		*ifipnext = ifi;			/* prev points to this new one */
		ifipnext = &ifi->ifi_next;	/* pointer to next one goes here */

		ifi->ifi_flags = flags;		/* IFF_xxx values */
		memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME);
		ifi->ifi_name[IFI_NAME-1] = '\0';
		/* If the sockaddr_dl is from a different interface, ignore it */
		if (sdlname == NULL || strcmp(sdlname, ifr->ifr_name) != 0)
			idx = 0;
		ifi->ifi_index = idx;

		switch (ifr->ifr_addr.sa_family) {
		case AF_INET:
			sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
			ifi->ifi_addr = (struct sockaddr*)calloc(1, sizeof(struct sockaddr_in));
			memcpy(ifi->ifi_addr, sinptr, sizeof(struct sockaddr_in));

#ifdef	SIOCGIFBRDADDR
			if (flags & IFF_BROADCAST) {
				ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);
				sinptr = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
				ifi->ifi_brdaddr = (struct sockaddr*)calloc(1, sizeof(struct sockaddr_in));
				memcpy(ifi->ifi_brdaddr, sinptr, sizeof(struct sockaddr_in));
			}
#endif
			/*
                         * LE03 - Donato Capitella   
                         */
                        break;
		case AF_INET6:
			sin6ptr = (struct sockaddr_in6 *) &ifr->ifr_addr;
			ifi->ifi_addr = (struct sockaddr*)calloc(1, sizeof(struct sockaddr_in6));
			memcpy(ifi->ifi_addr, sin6ptr, sizeof(struct sockaddr_in6));

			break;

		default:
			break;
		}
	}
	free(buf);
	return(ifihead);	/* pointer to first structure in linked list */
}
/* end get_ifi_info4 */

/* include free_ifi_info */
void free_ifi_info(struct ifi_info *ifihead)
{
	struct ifi_info	*ifi, *ifinext;

	for (ifi = ifihead; ifi != NULL; ifi = ifinext) {
		if (ifi->ifi_addr != NULL)
			free(ifi->ifi_addr);
		if (ifi->ifi_brdaddr != NULL)
			free(ifi->ifi_brdaddr);
		ifinext = ifi->ifi_next;	/* can't fetch ifi_next after free() */
		free(ifi);					/* the ifi_info{} itself */
	}
}
/* end free_ifi_info */

int sock_cmp_port(const struct sockaddr *sa1, const struct sockaddr *sa2, socklen_t salen)
{
	if (sa1->sa_family != sa2->sa_family)
		return(-1);

	switch (sa1->sa_family) {
	case AF_INET: {
		return(memcmp( &((struct sockaddr_in *) sa1)->sin_port,
					   &((struct sockaddr_in *) sa2)->sin_port,
					   sizeof(((struct sockaddr_in *)sa1)->sin_port)));
	}

#ifdef	IPV6
	case AF_INET6: {
		return(memcmp( &((struct sockaddr_in6 *) sa1)->sin6_port,
					   &((struct sockaddr_in6 *) sa2)->sin6_port,
					   sizeof(((struct sockaddr_in6 *)sa1)->sin6_port)));
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		return(-1);
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		return(-1);		/* no idea what to compare here ? */
	}
#endif
	}
    return (-1);
}

int sock_cmp_addr(const struct sockaddr *sa1, const struct sockaddr *sa2, socklen_t salen)
{
	if (sa1->sa_family != sa2->sa_family)
		return(-1);

	switch (sa1->sa_family) {
	case AF_INET: {
		return(memcmp( &((struct sockaddr_in *) sa1)->sin_addr,
					   &((struct sockaddr_in *) sa2)->sin_addr,
					   sizeof(struct in_addr)));
	}

#ifdef	IPV6
	case AF_INET6: {
		return(memcmp( &((struct sockaddr_in6 *) sa1)->sin6_addr,
					   &((struct sockaddr_in6 *) sa2)->sin6_addr,
					   sizeof(struct in6_addr)));
	}
#endif

#ifdef	AF_UNIX
	case AF_UNIX: {
		return(strcmp( ((struct sockaddr_un *) sa1)->sun_path,
					   ((struct sockaddr_un *) sa2)->sun_path));
	}
#endif

#ifdef	HAVE_SOCKADDR_DL_STRUCT
	case AF_LINK: {
		return(-1);		/* no idea what to compare here ? */
	}
#endif
	}
    return (-1);
}

int mcast_join(int sockfd, const struct sockaddr *grp, socklen_t grplen, const char *ifname, u_int ifindex)
{
/* include mcast_join2 */
	switch (grp->sa_family) {
	case AF_INET: {
		struct ip_mreq		mreq;
		struct ifreq		ifreq;

		memcpy(&mreq.imr_multiaddr,
			   &((const struct sockaddr_in *) grp)->sin_addr,
			   sizeof(struct in_addr));

		if (ifindex > 0) {
			if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
				errno = ENXIO;	/* i/f index not found */
				return(-1);
			}
			goto doioctl;
		} else if (ifname != NULL) {
			strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
doioctl:
			if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
				return(-1);
			memcpy(&mreq.imr_interface,
				   &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
				   sizeof(struct in_addr));
		} else
			mreq.imr_interface.s_addr = htonl(INADDR_ANY);

		return(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
						  &mreq, sizeof(mreq)));
	}
/* end mcast_join2 */

/* include mcast_join3 */
#ifdef	IPV6
#ifndef	IPV6_JOIN_GROUP		/* APIv0 compatibility */
#define	IPV6_JOIN_GROUP		IPV6_ADD_MEMBERSHIP
#endif
	case AF_INET6: {
		struct ipv6_mreq	mreq6;

		memcpy(&mreq6.ipv6mr_multiaddr,
			   &((const struct sockaddr_in6 *) grp)->sin6_addr,
			   sizeof(struct in6_addr));

		if (ifindex > 0) {
			mreq6.ipv6mr_interface = ifindex;
		} else if (ifname != NULL) {
			if ( (mreq6.ipv6mr_interface = if_nametoindex(ifname)) == 0) {
				errno = ENXIO;	/* i/f name not found */
				return(-1);
			}
		} else
			mreq6.ipv6mr_interface = 0;

		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
						  &mreq6, sizeof(mreq6)));
	}
#endif

	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
}
/* end mcast_join3 */

int						/* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = send(fd, ptr, nleft, 0)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

int
Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		return -1;
	return nbytes;
}
