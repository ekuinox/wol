#pragma once

#ifndef __CLIENT_H_
#define __CLIENT_H_

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <functional>
#include <array>
#include <iostream>

namespace trau
{
	auto arp(const char *host) -> std::array<u_int8_t, 6>;
	auto arp(const char *host, bool isString) -> std::string;
	auto mactoa(const std::string& str)-> std::array<u_int8_t, 6>;
	auto wol(const struct in_addr ip, const std::array<u_int8_t, 6> mac) -> void;
	auto wol(const char *host) -> void;
	auto wol(const char *ipAddr, const char *macAddr) -> void;
	auto socketDo(int __domain, int __type, int __protocol, std::function<void(const int&)> func) -> void;
}

#endif // end __CLIENT_H_