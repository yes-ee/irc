#ifndef SERVER_HPP
#define SERVER_HPP

#include "../inc/Channel.hpp"
#include "../inc/Client.hpp"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <fcntl.h>
#include <sys/event.h>
#include <algorithm>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <sys/time.h>
#include <sys/types.h>
#include <exception>
#include <sstream>

class Server {
	private:
		int port;
		int password;
		int server_socket;
		int kq;
		std::string servername;
		std::vector<struct kevent>	change_list;
		struct kevent	*curr_event;
		std::vector<char>	buffer;
		struct kevent	event_list[1024];
		std::map<std::string, Channel> channels;
		std::map<int, Client> clients;
		std::map<int, std::string> send_data;
	public:
		Server();
		Server(int port, int password);
		~Server();
		void init();
		void setPort(int port);
		int getPort() const;
		void setServerSocket(int server_socket);
		int getServerSocket() const;
		void setKq(int kq);
		int getKq() const;
		void setPassword(int password);
		int getPassword() const;
		void addChannel(std::string& channel);
		std::map<std::string, Channel> getChannels() const;
		std::map<int, Client> getClients() const;
		void changeEvent(std::vector<struct kevent>& change_list, 
			uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
		void run();
		void disconnectClient(int client_fd);
		void parseData(Client& client);
		std::string makeSendData(Client& client, std::string& cmd);
		std::string handleUser(Client& client, std::string& cmd, std::stringstream& buffer_stream);
		std::string handleJoin(Client& client, std::string& cmd);

	class	bindError: public std::exception
	{
		public:
			virtual const char	*what() const throw()
			{
				return ("bind error");
			}
	};
	class	listenError: public std::exception
	{
		public:
			virtual const char	*what() const throw()
			{
				return ("listen error");
			}
	};
	class	kqueueError: public std::exception
	{
		public:
			virtual const char	*what() const throw()
			{
				return ("kqueue error");
			}
	};
	class	keventError: public std::exception
	{
		public:
			virtual const char	*what() const throw()
			{
				return ("kevent error");
			}
	};
	class	acceptError: public std::exception
	{
		public:
			virtual const char	*what() const throw()
			{
				return ("accept error");
			}
	};
	class	readError: public std::exception
	{
		public:
			virtual const char	*what() const throw()
			{
				return ("read error");
			}
	};
	class	unkownError: public std::exception
	{
		public:
			virtual const char	*what() const throw()
			{
				return ("unkown error");
			}
	};
};

#endif