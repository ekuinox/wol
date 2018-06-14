#include <iostream>
#include "client.hpp"

auto main(int argc, char **argv) -> int
{
	if (argc < 3)
	{
		std::cout << "Usage: wol <IPAddress> <MACAddress>" << std::endl;
		return 0;
	}

	const auto hostIp = argv[1];
	const auto hostMac = argv[2];

	try
	{
		trau::wol(hostIp, hostMac);
	}
	catch (std::runtime_error e)
	{
		std::cout << e.what() << std::endl;
	}

	return 0;
}
