#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "vector"
#include "set"
#include "iostream"
#include "string"
#include "sstream"
#include "Client.hpp"

class Channel {
	private:
		int num;
		int password;
		int user_limit;
		Client owner;
		std::string topic;
		std::vector<Client> operators;
		std::vector<Client> clients;
		std::vector<Client> bans;
		std::set<char> modes;
	public:
		Channel();
		~Channel();
		void setNum(int num);
		int getNum() const;
		void setPassword(int password);
		int getPassword() const;
		void setOwner(Client& client);
		Client getOwner() const;
		void addOperator(const Client& client);
		std::vector<Client> getOperators() const;
		void joinClient(const Client& client);
		void addBan(const Client& client);
		bool checkBan(const Client& client);
		bool checkClient(const Client& client);
		bool isOperator(const Client& client);
		std::vector<Client> getClients() const;
		bool validateCommand(std::vector<std::string>& mode_cmd, std::string& command);
		void setMode(std::string& command);
};

#endif