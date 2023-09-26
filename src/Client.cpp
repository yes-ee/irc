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
	this->reg = false;
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

void Client::joinRequest(Channel& channel)
{
	channel.joinClient(*this);
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