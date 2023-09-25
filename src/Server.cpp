#include "../inc/Server.hpp"

Server::Server()
{

}

Server::~Server()
{

}

void Server::setPort(int port)
{
	this->port = port;
}

int Server::getPort() const
{
	return this->port;
}

void Server::setPassword(int password)
{
	this->password = password;
}

int Server::getPassword() const
{
	return this->password;
}

void Server::addChannel(const Channel& channel)
{
	this->channels.push_back(channel);
}

std::vector<Channel> Server::getChannels() const
{
	return this->channels;
}

void Server::addClient(const Client& client)
{
	this->clients.push_back(client);
}

std::vector<Client> Server::getClients() const
{
	return this->clients;
}
