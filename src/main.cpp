#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Invalid Arguments Number" << std::endl;
		return 1;
	}

	int port = atoi(argv[1]);
	std::string password(argv[2]);

	if (port < 1024 || port > 49151)
	{
		std::cerr << "Bad port number" << std::endl;
		return 1;
	}

	if (password.empty())
	{
		std::cerr << "Password empty" << std::endl;
		return 1;
	}

	if (password.length() > 10)
	{
		std::cerr << "Password too long" << std::endl;
		return 1;
	}

	Server server(port, password);
	try
	{
		server.init();
		server.run();
	}
	catch(const std::exception& e)
	{
		server.deleteChannel();
		std::cerr << e.what() << '\n';
	}

}