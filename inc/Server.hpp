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
#include <set>

// error
#define ERR_TOOMANYCHANNELS(user, channel)				"405 " + user + " " + channel + " :You have joined too many channels"
#define ERR_NONICKNAMEGIVEN(user)			            "431 " + user + " :Nickname not given"
#define ERR_NICKNAMEINUSE(user)				            "433 " + user + " " + user + " :Nickname is already in use"
#define ERR_NEEDMOREPARAMS(source, command)	      		"461 " + source + " " + command + " :Not enough parameters"
#define ERR_ALREADYREGISTRED(source)		         	"462 " + source + " :You may not register"
#define ERR_PASSWDMISMATCH(user)			            "464 " + user + " :Password incorrect"
#define ERR_NOORIGIN(user)								"465 " + user + " :No origin specified"
#define ERR_CHANNELISFULL(user, channel)				"471 " + user + " " + channel + " :Cannot join channel (+1)"
#define ERR_INVITEONLYCHAN(user, channel)				"473 " + user + " " + channel + " :Cannot join channel (+i)"
#define ERR_BANNEDFROMCHAN(user, channel)				"474 " + user + " " + channel + " :Cannot join channel (+b)"
#define ERR_BADCHANNELKEY(user, channel)				"475 " + user + " " + channel + " :Cannot join channel (+k)"
#define ERR_QUIT(user, message)							"ERROR :Closing link: (" + user + ") [Quit: " + message + "]"

// numeric
#define RPL_WELCOME(user)								"001 " + user + " :Welcome to the happyirc network " + user + "!"
#define RPL_TOPIC(user, channel, topic)					"332 " + user + " " + channel + " :" + topic
#define RPL_TOPICWHOTIME(user, channel, nick, setat)	"333 " + user + " " + channel + " " + nick + " " + setat
#define RPL_NAMREPLY(user, symbol, channel, users) 		"353 " + user + " " + symbol + " " + channel + " :" + users
#define RPL_ENDOFNAMES(user, channel)					"366 " + user + " " + channel + " :End of /NAMES list."

// command
#define RPL_PONG(user, ping)							":" + user + " PONG :" + ping
#define RPL_JOIN(user, channel)							":" + user + " JOIN :" + channel
#define RPL_QUIT(user, message)							":" + user + " QUIT :Quit: " + message



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
		std::map<std::string, Channel*> channels;
		std::map<int, Client> clients;
		std::map<std::string, Client> clients_by_name;
		std::map<int, std::string> send_data;
		std::set<int> close_client;
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
		std::map<std::string, Channel*> getChannels() const;
		std::map<int, Client> getClients() const;
		void changeEvent(std::vector<struct kevent>& change_list, 
			uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata);
		void run();
		void disconnectClient(int client_fd);
		void parseData(Client& client);
  
		std::string makeCRLF(const std::string& cmd);
		std::string handleJoin(Client& client, std::stringstream& buffer_stream);
		//std::string handleWho(Client& client, std::stringstream buffer_stream);
		std::string handlePass(Client& client, std::stringstream& buffer_stream);
		std::string handleNick(Client& client, std::stringstream& buffer_stream);
		std::string handleUser(Client& client, std::stringstream& buffer_stream);
		std::string handlePingpong(Client& client, std::stringstream& buffer_stream);
		std::string handleQuit(Client& client, std::stringstream& buffer_stream);
		Channel* createChannel(std::string& channel_name, std::string& key, Client& client);
		void broadcast(std::string& channel_name, const std::string& msg);

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