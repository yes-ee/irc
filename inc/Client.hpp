#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <string>
#include <map>

#define CLIENT_CHANLIMIT 3

class Channel;

class Client {
	private:
		int socket;
		bool reg;
		bool close;
		std::string buffer;
		std::string pre_cmd;
		std::string nickname;
		std::string username;
		std::string hostname;
		std::string servername;
		std::string realname;
		std::map<std::string, Channel> channels;

	public:
		Client();
		~Client();
		Client(int socket);
		void setSocket(int socket);
		int getSocket() const;
		void setRegister(bool flag);
		bool getRegister() const;
		void setClose(bool flag);
		bool getClose() const;
		void setNickname(std::string& nickname);
		std::string getNickname() const;
		void setUsername(std::string& username);
		std::string getUsername() const;
		void setHostname(std::string& hostname);
		std::string getHostname() const;
		void setServername(std::string& servername);
		std::string getServername() const;
		void setRealname(std::string& realname);
		std::string getRealname() const;
		void addBuffer(std::string data);
		std::string getBuffer() const;
		void clearBuffer();
		void setPreCmd(std::string& pre_cmd);
		std::string getPreCmd() const;
		std::string getPrefix() const;
		void joinChannel(const Channel* channel);
		std::map<std::string, Channel> getChannels() const;
		void leaveChannel(Channel *channel);
};

		bool operator==(const Client& lhs, const Client& rhs);

#endif