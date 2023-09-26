#include "../inc/Channel.hpp"

Channel::Channel()
{

}

Channel::~Channel()
{

}

Channel::Channel(std::string& name, std::string& key, Client& client) : name(name), owner(client), user_limit(2), invite_mode(false)
{
	std::string nick = client.getNickname();

	if (!key.empty())
	{
		this->password = key;
	}
	this->users[nick] = client;
	this->auth[nick] = OWNER;
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

void Channel::setInviteMode(bool flag)
{
	this->invite_mode = flag;
}

bool Channel::getInviteMode() const
{
	return this->invite_mode;
}


void Channel::setUserLimit(int limit)
{
	this->user_limit = limit;
}

int Channel::getUserLimit() const
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
	if (this->auth[client.getNickname()] >= OPERATOR)
		return true;
	return false;
}

void Channel::joinClient(const Client& client)
{
	if (checkBan(client))
		throw banError();

	std::string name = client.getNickname();
	this->users[name] = client;
	this->auth[name] = COMMON;
}

void Channel::addBan(const Client& client)
{
	std::string name = client.getNickname();

	this->auth.erase(name);
	this->users.erase(name);
	this->ban.push_back(name);
	// quit 처리
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

bool Channel::validateCommand(std::vector<std::string>& mode_cmd, std::string& command)
{
	if (command.length() < 2)
		return false;
	std::string flag(1,command[0]);
	std::string mode(1,command[1]);
	if (flag != "+" && flag != "-")
		return false;
	std::stringstream ss(command);

	std::string word;
	std::vector<std::string> words;
	while (getline(ss, word, ' ')){
        words.push_back(word);
    }
	if (flag == "-" && words.size() > 1)
		return false;
	if (words.size() > 2)
		return false;
	if (mode != "i" && mode != "t" && mode != "k" && mode != "o")
		return false;
	if (mode == "l" || mode == "t")
	{
		if (words[1].length() > 9)
			return false;
		if (mode == "l")
		{
			for (int i = 0; i < words[1].length(); i++)
			{
				if (!std::isdigit(words[1][i]))
					return false;
			}
		}
	}
	mode_cmd.push_back(flag);
	mode_cmd.push_back(mode);
	if (words.size() == 2)
		mode_cmd.push_back(words[1]);
	return true;
}

void Channel::setMode(std::string& command)
{
	std::vector<std::string> mode_cmd;

	if (!validateCommand(mode_cmd, command))
		return ;
	
	// this->modes.insert(mode);
}