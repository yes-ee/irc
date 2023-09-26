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

#define ERR_NONICKNAMEGIVEN(user)			"431 " + user + " :Nickname not given"
#define ERR_NICKNAMEINUSE(user)				"433 " + user + " " + user + " :Nickname is already in use"
#define RPL_WELCOME(user)					"001 " + user + " :Welcome to the happyirc network " + user + "!"
#define RPL_PONG(user, ping)				":" + user + " PONG :" + ping
#define RPL_QUIT(user, message)				":" + user + " QUIT :Quit: " + message
#define ERR_QUIT(user, message)				"ERROR :Closing link: (" + user + ") [Quit: " + message + "]"
#define ERR_NOORIGIN()						":No origin specified"
#define ERR_ALREADYREGISTRED(source)		"462 " + source + " :You may not register"
#define ERR_NEEDMOREPARAMS(source, command)	"461 " + source + " " + command + " :Not enough parameters"
#define ERR_PASSWDMISMATCH(user)			"464 " + user + " :Password incorrect"

class Server {
	private:
		int port;
		std::string password;
		int server_socket;
		int kq;
		std::string servername;
		std::vector<struct kevent>	change_list;
		struct kevent	*curr_event;
		std::vector<char>	buffer;
		struct kevent	event_list[1024];
		std::map<std::string, Channel> channels;
		std::map<int, Client> clients;
		std::map<std::string, Client> clients_by_name;
		std::map<int, std::string> send_data;
	public:
		Server();
		Server(int port, std::string password);
		~Server();
		void init();
		void setPort(int port);
		int getPort() const;
		void setServerSocket(int server_socket);
		int getServerSocket() const;
		void setKq(int kq);
		int getKq() const;
		void setPassword(std::string password);
		std::string getPassword() const;
		void addChannel(std::string& channel);
		std::map<std::string, Channel> getChannels() const;
		std::map<int, Client> getClients() const;
		void changeEvent(std::vector<struct kevent>& change_list, 
			uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
		void run();
		void disconnectClient(int client_fd);
		void parseData(Client& client);
		std::string handlePass(Client& client, std::stringstream& buffer_stream);
		std::string handleNick(Client& client, std::stringstream& buffer_stream);
		std::string makeCRLF(std::string& cmd);
		std::string handleUser(Client& client, std::stringstream& buffer_stream);
		std::string handlePingpong(Client& client, std::stringstream& buffer_stream);
		std::string handleQuit(Client& client, std::stringstream& buffer_stream);
		// std::string handleWho(Client& client, std::stringstream buffer_stream);

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