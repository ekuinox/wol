#include <iostream>
#include "client.hpp"

auto main(int argc, char **argv) -> int
{
	if (argc == 1)
	{
		std::cout << "Usage: wol <IPAddress>" << std::endl;
		return 0;
	}

	const auto hostIp = argv[1];

	std::cout << "IP " << hostIp << std::endl;

	auto hostMac = trau::arp(hostIp, true);

	std::cout << "Mac: " << hostMac << std::endl;

	try
	{
		trau::wol(hostIp);
	}
	catch (std::runtime_error e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}
