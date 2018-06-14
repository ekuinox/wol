#include "client.hpp"

// ���Ȃ�Q�l�ɂ��� https://gitlab.ais.cmc.osaka-u.ac.jp/ishigaki/arping/blob/ab6a5c73ecb7239d4c7d95c89740948d56ccdbad/myarping.c
// IP����MAC���擾����
auto trau::arp(const char * host) ->std::array<u_int8_t, 6>
{
	constexpr auto iface = "br0";
	constexpr auto timeout = 5;
	
	auto err_cnt = 0;
	std::array<u_int8_t, 6> result;
	
	do
	{
		try
		{
			socketDo(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ARP), [&](const int &_sock)
			{
				/* ���M�ݒ� */
				struct sockaddr_ll sockaddr;
				memset(&sockaddr, 0, sizeof(sockaddr));
				sockaddr.sll_family = AF_PACKET;
				sockaddr.sll_protocol = htons(ETH_P_ARP);
				sockaddr.sll_ifindex = if_nametoindex(iface);
				sockaddr.sll_halen = ETH_ALEN;
				memset(&sockaddr.sll_addr, 0xff, 6);

				/* br0��MAC�A�h���X�擾 */
				struct ifreq mac_interface;
				memset(&mac_interface, 0, sizeof(mac_interface));
				socketDo(AF_INET, SOCK_DGRAM, 0, [&mac_interface](const int &s)
				{
					mac_interface.ifr_addr.sa_family = AF_INET;
					strncpy(mac_interface.ifr_name, iface, IFNAMSIZ - 1);
					if (ioctl(s, SIOCGIFHWADDR, &mac_interface) < 0) throw std::runtime_error("get mac adder");
				});

				/* br0ip�A�h���X�擾*/
				struct ifreq ip_interface;
				memset(&ip_interface, 0, sizeof(ip_interface));
				socketDo(AF_INET, SOCK_DGRAM, 0, [&ip_interface](const int &s)
				{
					ip_interface.ifr_addr.sa_family = AF_INET;
					strncpy(ip_interface.ifr_name, iface, IFNAMSIZ - 1);
					if (ioctl(s, SIOCGIFADDR, &ip_interface) < 0) throw std::runtime_error("get ip adder");
				});

				/* arp�p�P�b�g�̐ݒ� */
				struct ether_arp arpPacket;
				memset(&arpPacket, 0, sizeof(arpPacket));
				arpPacket.arp_hrd = htons(1);
				arpPacket.arp_pro = htons(ETH_P_IP);
				arpPacket.arp_hln = 6;
				arpPacket.arp_pln = 4;
				arpPacket.arp_op = htons(ARPOP_REQUEST);
				memset(&arpPacket.arp_tha, 0xff, 6);
				memcpy(arpPacket.arp_sha, mac_interface.ifr_hwaddr.sa_data, 6);
				inet_aton((const char*)inet_ntoa(((struct sockaddr_in *)&ip_interface.ifr_addr)->sin_addr), (struct in_addr *)&arpPacket.arp_spa);
				inet_aton(host, (struct in_addr *)&arpPacket.arp_tpa);

				/* arpPacket���M */
				if (sendto(_sock, (char *)&arpPacket, sizeof(arpPacket), 0, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) throw std::runtime_error("sendto err");

				struct ether_arp arpReply;
				memset(&arpReply, 0, sizeof(arpReply));

				/* arp������arpReply�Ɋi�[ */
				if (recvfrom(_sock, (char *)&arpReply, sizeof(arpReply), 0, NULL, NULL) < 0)
				{
					throw std::runtime_error("recv err");
				}
				else
				{
					for (size_t i = 0; i < result.max_size(); ++i) result[i] = arpReply.arp_sha[i];
					err_cnt += timeout;
				}
			});
		}
		catch (std::runtime_error e)
		{
			err_cnt += 1;
		}
	} while (err_cnt < timeout);
	
	return result;
}

// IP����MAC��::�Ȍ`���ɂ��ĕԂ�
auto trau::arp(const char * host, bool isString) -> std::string
{
	auto mac = arp(host);
	if (mac.size() != mac.max_size()) return nullptr;
	char str[17];
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
	);
	return std::string(str);
}

// 00:00:00:00:00:00��z��ɂ��ĕԂ�
auto trau::mactoa(const std::string& str) -> std::array<u_int8_t, 6>
{
	std::array<u_int8_t, 6> result;
	for (size_t i = 0; i < result.max_size(); ++i)
	{
		result[i] = static_cast<u_int8_t>(strtol(str.substr(i * 3, 2).c_str(), NULL, 16));
	}
	return result;
}

auto trau::wol(const in_addr ip, const std::array<u_int8_t, 6> mac) -> void
{
	constexpr auto PACKET_BUF = 17 * 6;
	constexpr auto REMOTE_PORT = 9;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(REMOTE_PORT);
	addr.sin_addr = ip;

	unsigned char packet[PACKET_BUF];
	for (auto i = 0; i < 6; i++)
	{
		packet[i] = 0xFF;
	}

	for (auto i = 1; i <= 16; i++)
	{
		for (auto j = 0; j < 6; j++)
		{
			packet[i * 6 + j] = mac[j];
		}
	}

	socketDo(AF_INET, SOCK_DGRAM, IPPROTO_UDP, [&](const int& sock)
	{
		if (sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *) &addr, sizeof(addr)) < 0) throw std::runtime_error("Cannot send data...!\n");
	});
}

// �Q�l�ɂ��� https://github.com/timofurrer/WOL/tree/master/src
// IP����WoL������ // �łĂ˂���o�J
auto trau::wol(const char * host) -> void
{
	struct in_addr ip;
	if (inet_aton(host, &ip) == 0) throw std::runtime_error("Invalid remote ip address given: %s ...!\n");

	auto mac = arp(host);
	if (mac.size() != mac.max_size()) throw std::runtime_error("could not resolve ip");

	wol(ip, mac);
}

auto trau::wol(const char * idAddr, const char * macAddr) -> void
{
	struct in_addr ip;
	if (inet_aton(idAddr, &ip) == 0) throw std::runtime_error("Invalid remote ip address given: %s ...!\n");

	auto mac = mactoa(macAddr);

	wol(ip, mac);
}

// close�Y�ꂵ�Ȃ��悤�ɂ���������
auto trau::socketDo(int __domain, int __type, int __protocol, std::function<void(const int&)> func) -> void
{
	auto sock = socket(__domain, __type, __protocol);
	if (sock < 0) throw errno;
	func(sock);
	close(sock);
}
