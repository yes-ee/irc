#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <ctime>
#include "Client.hpp"

#define OWNER		1
#define OPERATOR	2
#define COMMON		3

#define	BAN			4

class Channel {
	private:
		std::string name;
		std::string password;
		long long user_limit;
		time_t create_time;
		Client owner;
		std::string topic;
		std::map<std::string, Client> users;
		std::vector<std::string> ban;
		std::set<char> modes;
		std::string topic_set_time;
		std::string topic_set_user;
		std::vector<Client> invited;
		std::map<std::string, int> auth;
	public:
		Channel();
		~Channel();
		Channel(std::string& name, std::string& key, Client& client);
		void setName(std::string& name);
		std::string getName() const;
		void setPassword(std::string& password);
		std::string getPassword() const;
		void setTopic(Client& client, std::string& topic);
		std::string getTopic() const;
		bool getInviteMode() const;
		void setOperator(const Client& client);
		bool isOperator(const Client& client);
		void setUserLimit(long long limit);
		long long getUserLimit() const;
		void setCreateTime(long long time);
		long long getCreateTime() const;
		void setOwner(Client& client);
		Client getOwner() const;
		void joinClient(Client& client, int auth);
		void addInvited(const Client& client);
		bool isInvite(const Client& client);
		void addBan(const Client& client);
		bool checkBan(const Client& client);
		std::map<std::string, Client> getUsers() const;
		std::set<char> getModes() const;
		std::string getModeString() const;
		bool findMode(char mode);
		void addMode(char mode);
		void eraseMode(char mode);
		void deleteClient(std::string nickname);
		void setTopicTime();
		std::string getTopicTime() const;
		std::string getTopicUser() const;
		void changeAuth(int auth, const Client &client);
		std::map<std::string, int> getAuth() const;

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