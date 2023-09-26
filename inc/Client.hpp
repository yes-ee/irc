#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "iostream"
#include "string"
#include "vector"

class Channel;

class Client {
	private:
		int socket;
		std::string buffer;
		std::string nickname;
		std::string hostname;
		std::string username;
		std::string realname;
		std::vector<Channel> channels;

	public:
		Client();
		~Client();
		Client(int socket);
		void setSocket(int socket);
		int getSocket() const;
		void setNickname(std::string& nickname);
		std::string getNickname() const;
		void setHostname(std::string& hostname);
		std::string getHostname() const;
		void setUsername(std::string& username);
		std::string getUsername() const;
		void setRealname(std::string& realname);
		std::string getRealname() const;
		void joinRequest(Channel& channel);
		void addBuffer(std::string data);
		std::string getBuffer() const;
		void clearBuffer();
};

		bool operator==(const Client& lhs, const Client& rhs);

#endif