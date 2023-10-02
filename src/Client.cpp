#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

Client::Client()
{

}

Client::~Client()
{

}

Client::Client(int socket) : socket(socket)
{
	this->buffer = "";
	this->pre_cmd = "";
	this->reg = false;
	this->close = false;
	this->nickname = "client" + std::to_string(socket);
	this->hostname = "127.0.0.1";
	this->realname = "root";
	this->servername = "root";
	this->username = "root";
}

void Client::setSocket(int socket)
{
	this->socket = socket;
}

int Client::getSocket() const
{
	return this->socket;
}

void Client::setRegister(bool flag)
{
	this->reg = flag;
}

bool Client::getRegister() const
{
	return this->reg;
}

bool Client::getClose() const
{
	return this->close;
}
void Client::setClose(bool flag)
{
	this->close = flag;
}

void Client::setNickname(std::string& nickname)
{
	this->nickname = nickname;
}

std::string Client::getNickname() const
{
	return this->nickname;
}

void Client::setServername(std::string& servername)
{
	this->servername = servername;
}

std::string Client::getServername() const
{
	return this->servername;
}

void Client::setHostname(std::string& hostname)
{
	this->hostname = hostname;
}

std::string Client::getHostname() const
{
	return this->hostname;
}


void Client::setUsername(std::string& username)
{
	this->username = username;
}

std::string Client::getUsername() const
{
	return this->username;
}

void Client::setRealname(std::string& realname)
{
	this->realname = realname;
}

std::string Client::getRealname() const
{
	return this->realname;
}

void Client::addBuffer(std::string data)
{
	this->buffer += data;
}

std::string Client::getBuffer() const
{
	return this->buffer;
}

void Client::clearBuffer()
{
	this->buffer.clear();
}

void Client::setPreCmd(std::string& pre_cmd)
{
	this->pre_cmd = pre_cmd;
}

std::string Client::getPreCmd() const
{
	return this->pre_cmd;
}

bool operator==(const Client& lhs, const Client& rhs)
{
	return (lhs == rhs);
}

std::string Client::getPrefix() const
{
    std::string username = "!" + this->username;
    std::string hostname = "@" + this->hostname;

    return this->nickname + username + hostname;
}

void Client::joinChannel(const Channel* channel)
{
	this->channels[channel->getName()] = *channel;
}

void Client::leaveChannel(Channel *channel)
{
	this->channels.erase(channel->getName());
}

std::map<std::string, Channel> Client::getChannels() const
{
	return this->channels;
}
