#ifndef SERVER_HPP
#define SERVER_HPP

#include "../inc/Channel.hpp"
#include "../inc/Client.hpp"
#include "vector"
#include "iostream"
#include "string"

class Server {
	private:
		int port;
		int password;
		std::vector<Channel> channels;
		std::vector<Client> clients;
	public:
		Server();
		~Server();
		void setPort(int port);
		int getPort() const;
		void setPassword(int password);
		int getPassword() const;
		void addChannel(const Channel& channel);
		std::vector<Channel> getChannels() const;
		void addClient(const Client& client);
		std::vector<Client> getClients() const;
};

#endif