#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>

#include "q_stdinc.h"
#include "arch_def.h"
#include "net_sys.h"
#include "quakedef.h"
#include "net_defs.h"
#include "net_udp_if.h"

int UDP_ifQueryAddr (int sock, unsigned long request, struct ifreq *ifr, char *buf, int len)
{
	if (ioctl (sock, request, ifr) < 0)
	{
		Con_SafePrintf ("Warning, failed to query interface!\n");
		return 0;
	}

	if (!inet_ntop (AF_INET, &(((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr), buf, len))
	{
		Con_SafePrintf ("Failed to parse IP from interface %s!\n", ifr->ifr_name);
		return 0;
	}

	return 1;
}

int UDP_ifQueryInterfaces (struct ifr_t** ifr_buf)
{
	char buffer[8192];
	struct ifconf ifc;
	struct ifr_t *interfaces = NULL;

	int sock = 0;
	int i = 0;

	if((sock = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		Con_SafePrintf ("Error acquiring socket handle.\n");
		return 0;
	}

	ifc.ifc_len = sizeof(buffer);
	ifc.ifc_buf = buffer;

	if(ioctl (sock, SIOCGIFCONF, &ifc) < 0)
	{
		close (sock);
		Con_SafePrintf ("Failed to query available interfaces.\n");
		return 0;
	}

	int if_count = ifc.ifc_len / sizeof(struct ifreq);
	*ifr_buf = interfaces = calloc (if_count, sizeof(*interfaces));
	if (!interfaces)
	{
		close (sock);
		Con_SafePrintf ("Failed to allocate interfaces buffer!\n");
		return 0;
	}

	for (i=0; i<if_count; i++)
	{
		strcpy (interfaces[i].ifname, ifc.ifc_req[i].ifr_name);
		if (!UDP_ifQueryAddr (sock, SIOCGIFADDR,    &ifc.ifc_req[i], interfaces[i].ifaddr,  sizeof(interfaces[i].ifaddr)) ||
			!UDP_ifQueryAddr (sock, SIOCGIFBRDADDR, &ifc.ifc_req[i], interfaces[i].ifbroad, sizeof(interfaces[i].ifbroad)))
		{
			strcpy (interfaces[i].ifname, ""); /* failed interface query */
			continue;
		}
	}

	close (sock);
	return if_count;
}
