#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "iostream"
#include "string"
#include "vector"

class Channel;

class Client {
	private:
		int socket;
		std::string nickname;
		std::string username;
		std::vector<Channel> channels;

	public:
		Client();
		~Client();
		void setSocket(int socket);
		int getSocket() const;
		void setNickname(std::string& nickname);
		std::string getNickname() const;
		void setUsername(std::string& userName);
		std::string getUsername() const;
		void joinRequest(Channel& channel);
};

		bool operator==(const Client& lhs, const Client& rhs);

#endif