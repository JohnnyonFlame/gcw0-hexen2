#ifndef __net_udp_if_h
#define __net_udp_if_h

struct ifr_t {
	char ifname[IFNAMSIZ];
	char ifaddr[INET_ADDRSTRLEN];
	char ifbroad[INET_ADDRSTRLEN];
};

int UDP_ifQueryAddr (int sock, unsigned long request, struct ifreq *ifr, char *buf, int len);
int UDP_ifQueryInterfaces (struct ifr_t** ifr_buf);

#endif // __net_udp_if_h
