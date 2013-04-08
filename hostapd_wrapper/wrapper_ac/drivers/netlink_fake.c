/*
 * Netlink helper functions for driver wrappers
 * Copyright (c) 2002-2009, Jouni Malinen <j@w1.fi>
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


#include "includes.h"

#include "common.h"
#include "eloop.h"
#include "priv_netlink.h"
#include "netlink_fake.h"



struct netlink_data {
	struct netlink_config *cfg;
	int sock;
};


static void netlink_receive_link(struct netlink_data *netlink,
				 void (*cb)(void *ctx, struct ifinfomsg *ifi,
					    u8 *buf, size_t len),
				 struct nlmsghdr *h)
{
	
	return;
}


static void netlink_receive(int sock, void *eloop_ctx, void *sock_ctx)
{
	return;
}


struct netlink_data * netlink_init(struct netlink_config *cfg)
{

	struct netlink_data *netlink;
	struct sockaddr_nl local;

	netlink = os_zalloc(sizeof(*netlink));
	if (netlink == NULL)
		return NULL;

	netlink->cfg = cfg;

	netlink->sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	os_memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_groups = RTMGRP_LINK;

	eloop_register_read_sock(netlink->sock, netlink_receive, netlink,
				 NULL);

	return netlink;

}


void netlink_deinit(struct netlink_data *netlink)
{
	return;
}

int netlink_send_oper_ifla(struct netlink_data *netlink, int ifindex,
			   int linkmode, int operstate){
	
	return 0;
}
