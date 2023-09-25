#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

// irssi -c 127.0.0.1 -p 6667 -n nickname
int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Invalid Arguments Number" << std::endl;
		return 1;
	}

	int port = atoi(argv[1]); //int 아닌 경우 예외처리
	int password = atoi(argv[2]); // password 길이 제한 및 int 예외처리
	
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