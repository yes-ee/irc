#include "../inc/Client.hpp"
#include "../inc/Channel.hpp"

Client::Client()
{

}

Client::~Client()
{

}

void Client::setSocket(int socket)
{
	this->socket = socket;
}

int Client::getSocket() const
{
	return this->socket;
}

void Client::setNickname(std::string& nickname)
{
	this->nickname = nickname;
}

std::string Client::getNickname() const
{
	return this->nickname;
}

void Client::setUsername(std::string& userName)
{
	this->username = userName;
}

std::string Client::getUsername() const
{
	return this->username;
}

void Client::joinRequest(Channel& channel)
{
	channel.joinClient(*this);
}

bool operator==(const Client& lhs, const Client& rhs)
{
	return (lhs == rhs);
}