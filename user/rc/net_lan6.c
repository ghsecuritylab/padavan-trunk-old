/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>

#include <nvram/bcmnvram.h>
#include <shutils.h>

#include "rc.h"

#if defined (USE_IPV6)

void reset_lan6_vars(void)
{
	struct in6_addr addr6;
	char addr6s[INET6_ADDRSTRLEN] = {0};
	char *lan_addr6;
	int lan_size6;
	
	if (is_lan_addr6_static() == 1)
	{
		lan_addr6 = nvram_safe_get("ip6_lan_addr");
		lan_size6 = nvram_get_int("ip6_lan_size");
		if (*lan_addr6) {
			memset(&addr6, 0, sizeof(addr6));
			ipv6_from_string(lan_addr6, &addr6);
			inet_ntop(AF_INET6, &addr6, addr6s, INET6_ADDRSTRLEN);
			if (lan_size6 < 48 || lan_size6 > 80)
				lan_size6 = 64;
			sprintf(addr6s, "%s/%d", addr6s, lan_size6);
		}
	}

	nvram_set("lan_addr6", addr6s);
}

int is_lan_radv_on(void)
{
	int ipv6_type = get_ipv6_type();

	if (ipv6_type == IPV6_DISABLED)
		return -1;

	if (nvram_invmatch("ip6_lan_radv", "0"))
		return 1;

	return 0;
}

int is_lan_dhcp6s_on(void)
{
	int ipv6_type = get_ipv6_type();

	if (ipv6_type == IPV6_DISABLED)
		return -1;

	if (nvram_invmatch("ip6_lan_dhcp", "0"))
		return 1;

	return 0;
}

int is_lan_addr6_static(void)
{
	int ipv6_type = get_ipv6_type();

	if (ipv6_type == IPV6_DISABLED)
		return -1;

	if (ipv6_type == IPV6_6TO4 ||
	    ipv6_type == IPV6_6RD)
		return 0;

	if (nvram_match("ip6_lan_auto", "0") || 
	    ipv6_type == IPV6_NATIVE_STATIC || 
	    ipv6_type == IPV6_6IN4)
		return 1;
	
	return 0;
}

int store_lan_addr6(char *lan_addr6_new)
{
	char *lan_addr6_old;
	
	if (!lan_addr6_new)
		return 0;
	
	if (!(*lan_addr6_new))
		return 0;
	
	lan_addr6_old = nvram_safe_get("lan_addr6");
	if (strcmp(lan_addr6_new, lan_addr6_old) != 0) {
		nvram_set("lan_addr6", lan_addr6_new);
		return 1;
	}

	return 0;
}

void reload_lan_addr6(void)
{
	char *lan_addr6;
	
	clear_if_addr6(IFNAME_BR);
	lan_addr6 = nvram_safe_get("lan_addr6");
	if (*lan_addr6)
		doSystem("ip -6 addr add %s dev %s", lan_addr6, IFNAME_BR);
}

void clear_lan_addr6(void)
{
	clear_if_addr6(IFNAME_BR);
}

char *get_lan_addr6_host(char *p_addr6s)
{
	char *tmp = p_addr6s;
	char *lan_addr6 = nvram_safe_get("lan_addr6");
	if (*lan_addr6) {
		snprintf(p_addr6s, INET6_ADDRSTRLEN, "%s", lan_addr6);
		strsep(&tmp, "/");
		return p_addr6s;
	}

	return NULL;
}

char *get_lan_addr6_prefix(char *p_addr6s)
{
	/* force prefix len to 64 for SLAAC */
	struct in6_addr addr6;

	char *lan_addr6 = nvram_safe_get("lan_addr6");
	if (*lan_addr6) {
		if (ipv6_from_string(lan_addr6, &addr6) >= 0) {
			ipv6_to_net(&addr6, 64);
			if (inet_ntop(AF_INET6, &addr6, p_addr6s, INET6_ADDRSTRLEN) != NULL) {
				sprintf(p_addr6s, "%s/%d", p_addr6s, 64);
				return p_addr6s;
			}
		}
	}

	return NULL;
}

int reload_radvd(void)
{
	FILE *fp;
	int ipv6_type, is_dhcp6s_on, i_adv_per;
	char *adv_prefix, *adv_rdnss, *lan_addr6_prefix;
	char addr6s[INET6_ADDRSTRLEN], rdns6s[INET6_ADDRSTRLEN], wan_ifname[16] = {0};

	ipv6_type = get_ipv6_type();
	if (ipv6_type == IPV6_DISABLED)
		return 1;
	
	if (is_lan_radv_on() != 1)
		return 1;
	
	is_dhcp6s_on = is_lan_dhcp6s_on();
	i_adv_per = 60;
	adv_prefix = "::/64";
	adv_rdnss = get_lan_addr6_host(rdns6s);
	if (!adv_rdnss)
		adv_rdnss = nvram_safe_get("wan0_dns6");
	
	if (ipv6_type == IPV6_6TO4) {
		get_wan_ifname(wan_ifname);
		sprintf(addr6s, "0:0:0:%d::/%d", 1, 64);
		adv_prefix = addr6s;
	}
	else {
		lan_addr6_prefix = get_lan_addr6_prefix(addr6s);
		if (lan_addr6_prefix)
			adv_prefix = lan_addr6_prefix;
	}
	
	fp = fopen("/etc/radvd.conf", "w");
	if (!fp)
		return -1;
	
	fprintf(fp,
		"interface %s {\n"
		" IgnoreIfMissing on;\n"
		" AdvSendAdvert on;\n"			// (RA=ON)
		" AdvHomeAgentFlag off;\n"
		" AdvManagedFlag off;\n"		// (M=OFF)
		" AdvOtherConfigFlag %s;\n"
		" AdvDefaultLifetime %d;\n"
		" MaxRtrAdvInterval %d;\n"
		" prefix %s {\n"
		"  AdvOnLink on;\n"
		"  AdvAutonomous on;\n",
		IFNAME_BR,
		(is_dhcp6s_on > 0) ? "on" : "off",	// (O=ON/OFF)
		1800,
		i_adv_per,
		adv_prefix
	);
	
	if (ipv6_type == IPV6_6TO4) {
		fprintf(fp,
			"  AdvValidLifetime %d;\n"
			"  AdvPreferredLifetime %d;\n"
			"  Base6to4Interface %s;\n",
			600,
			240,
			wan_ifname
		);
	}
	
	fprintf(fp, " };\n");
	
	if (*adv_rdnss)
		fprintf(fp, " RDNSS %s {};\n", adv_rdnss);
	
	fprintf(fp, "};\n");
	
	fclose(fp);
	
	if (pids("radvd"))
		return doSystem("killall %s %s", "-SIGHUP", "radvd");
	
	return eval("/usr/sbin/radvd");
}

void stop_radvd(void)
{
	char* svcs[] = { "radvd", NULL };
	kill_services(svcs, 3, 1);
}

void restart_radvd(void)
{
	if (is_lan_radv_on() == 1)
		reload_radvd();
	else
		stop_radvd();
}

#endif
