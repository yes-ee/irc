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

	if (password.length() > 10)
	{
		std::cerr << "Password too long" << std::endl;
		return 1;
	}

	try
	{
		Server server(port, password);
		server.init();
		server.run();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

}