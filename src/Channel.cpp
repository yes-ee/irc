#include "../inc/Channel.hpp"

Channel::Channel()
{

}

Channel::~Channel()
{

}

Channel::Channel(std::string& name, std::string& key, Client& client) : name(name), owner(client), user_limit(3)
{
	std::string nick = client.getNickname();

	if (!key.empty())
	{
		this->password = key;
	}
	this->topic = "";
	this->users[nick] = client;
	this->auth[nick] = OWNER;
	this->modes.insert('n');
	this->modes.insert('t');
	this->create_time = time(NULL);
}

void Channel::setName(std::string& name)
{
	this->name = name;
}

std::string Channel::getName() const
{
	return this->name;
}

void Channel::setPassword(std::string& password)
{
	this->password = password;
}

std::string Channel::getPassword() const
{
	return this->password;
}

void Channel::setTopic(Client& client, std::string& topic)
{
	this->topic = topic;
	setTopicTime();
	this->topic_set_user = client.getPrefix();
}

std::string Channel::getTopic() const
{
	return this->topic;
}

bool Channel::getInviteMode() const
{
	if (this->modes.find('i') != this->modes.end())
		return true;
	return false;
}


void Channel::setUserLimit(long long limit)
{
	this->user_limit = limit;
}

long long Channel::getUserLimit() const
{
	return this->user_limit;
}

void Channel::setOwner(Client& client)
{
	this->owner = client;
}

Client Channel::getOwner() const
{
	return this->owner;
}

void Channel::setOperator(const Client& client)
{
	this->auth[client.getNickname()] = OPERATOR;
}

bool Channel::isOperator(const Client& client)
{
	if (this->auth[client.getNickname()] <= OPERATOR)
		return true;
	return false;
}

void Channel::joinClient(Client& client, int auth)
{
	if (checkBan(client))
		throw banError();
	if (auth == OWNER)
		this->setOwner(client);
	std::string name = client.getNickname();
	this->users[name] = client;
	this->auth[name] = auth;
}

void Channel::addBan(const Client& client)
{
	std::string name = client.getNickname();

	this->auth.erase(name);
	this->users.erase(name);
	this->ban.push_back(name);
}

bool Channel::checkBan(const Client& client)
{
	if (this->ban.size() > 0)
	{
		std::vector<std::string>::iterator it = find(this->ban.begin(), this->ban.end(), client.getNickname());
		if (it != this->ban.end())
			return true;
	}
	return false;
}

std::map<std::string, Client> Channel::getUsers() const
{
	return this->users;
}

std::set<char> Channel::getModes() const
{
	return this->modes;
}

std::string Channel::getModeString() const
{
	std::string mode = "+";

	for (std::set<char>::iterator it = this->modes.begin(); it != this->modes.end(); it++)
	{
		mode += *it;
	}
	return mode;
}

bool Channel::findMode(char mode)
{
	if(this->modes.find(mode) != this->modes.end())
		return true;
	return false;
}

void Channel::addMode(char mode)
{
	this->modes.insert(mode);
}

void Channel::eraseMode(char mode)
{
	this->modes.erase(mode);
}

void Channel::deleteClient(std::string nickname)
{
	this->users.erase(nickname);
	this->auth.erase(nickname);
}

void Channel::setTopicTime()
{
	time_t timer;
    struct tm* t;

	timer = time(NULL);
	t = localtime(&timer);
	this->topic_set_time = std::to_string(timer);
}

std::string Channel::getTopicTime() const
{
	return this->topic_set_time;
}

std::string Channel::getTopicUser() const
{
	return this->topic_set_user;
}

void Channel::setCreateTime(long long time)
{
	this->create_time = time;
}
long long Channel::getCreateTime() const
{
	return this->create_time;
}

void Channel::changeAuth(int auth, const Client& client)
{
	this->auth[client.getNickname()] = auth;
}

std::map<std::string, int> Channel::getAuth() const
{
	return this->auth;
}

void Channel::addInvited(const Client& client)
{
	this->invited.push_back(client);
}

bool Channel::isInvite(const Client& client)
{
	for (std::vector<Client>::iterator i_it = this->invited.begin(); i_it != this->invited.end(); i_it++)
	{
		if (i_it->getNickname() == client.getNickname())
		{
			this->invited.erase(i_it);
			return true;
		}
	}
	return false;
}