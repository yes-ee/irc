#include "../inc/Channel.hpp"

Channel::Channel()
{

}

Channel::~Channel()
{

}

Channel::Channel(std::string& name) : name(name)
{

}

void Channel::setName(std::string& name)
{
	this->name = name;
}

std::string Channel::getName() const
{
	return this->name;
}


void Channel::setPassword(int password)
{
	this->password = password;
}

int Channel::getPassword() const
{
	return this->password;
}

void Channel::setOwner(Client& client)
{
	this->owner = client;
}

Client Channel::getOwner() const
{
	return this->owner;
}


void Channel::addOperator(const Client& client)
{
	this->operators.push_back(client);
}

std::vector<Client> Channel::getOperators() const
{
	return this->operators;
}

void Channel::joinClient(const Client& client)
{
	if (checkClient(client))
		this->clients.push_back(client);
}

bool Channel::checkClient(const Client& client)
{
	if (checkBan(client))
		return false;
	return true;
}

void Channel::addBan(const Client& client)
{
	this->bans.push_back(client);
}

bool Channel::checkBan(const Client& client)
{
	return false;
}

std::vector<Client> Channel::getClients() const
{
	return this->clients;
}

bool Channel::isOperator(const Client& client)
{
	if (find(this->operators.begin(), this->operators.end(), client) != this->operators.end())
		return true;
	return false;
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