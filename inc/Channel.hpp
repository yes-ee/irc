#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "vector"
#include "set"
#include "map"
#include "iostream"
#include "string"
#include "sstream"
#include "Client.hpp"
#include "algorithm"

#define OWNER		1
#define OPERATOR	2
#define COMMON		3

#define	BAN			4

class Channel {
	private:
		
		std::string name;
		std::string password;
		bool invite_mode;
		int user_limit;
		Client owner;
		std::string topic;
		std::map<std::string, Client> users;
		std::map<std::string, int> auth;
		std::vector<std::string> ban;
		std::set<char> modes;
	public:
		Channel();
		~Channel();
		Channel(std::string& name, std::string& key, Client& client);
		void setName(std::string& name);
		std::string getName() const;
		void setPassword(std::string& password);
		std::string getPassword() const;
		void setTopic(std::string& topic);
		std::string getTopic() const;
		void setInviteMode(bool flag);
		bool getInviteMode() const;
		void setOperator(const Client& client);
		bool isOperator(const Client& client);
		void setUserLimit(int limit);
		int getUserLimit() const;
		void setOwner(Client& client);
		Client getOwner() const;
		void joinClient(const Client& client);
		void addBan(const Client& client);
		bool checkBan(const Client& client);
		std::map<std::string, Client> getUsers() const;
		bool validateCommand(std::vector<std::string>& mode_cmd, std::string& command);
		void setMode(std::string& command);
		std::set<char> getModes() const;
		std::string getModeString() const;
		void deleteClient(std::string nickname);

		class	banError: public std::exception
		{
			public:
				virtual const char	*what() const throw()
			{
				return ("banned client from this channel");
			}
	};
};

#endif