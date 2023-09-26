#include "../inc/Server.hpp"
#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

// docker run -d --name ubuntu -p 80:80 -it --privileged ubuntu:20.04
// irssi -c 10.12.6.3 -p 8080 -n mynick
int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Invalid Arguments Number" << std::endl;
		return 1;
	}

	int port = atoi(argv[1]); //int 아닌 경우 예외처리
	std::string password(argv[2]); // password 길이 제한
	
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