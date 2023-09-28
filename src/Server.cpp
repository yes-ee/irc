#include "../inc/Server.hpp"

Server::Server()
{

}

Server::~Server()
{

}

Server::Server(int port, std::string password) : port(port), password(password), servername("happyirc")
{

}

void Server::init()
{
	this->server_socket = socket(AF_INET, SOCK_STREAM, 0);
	const int value = 1;
	setsockopt(this->server_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	sockaddr_in	server_address;

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(this->port);

	if (bind(this->server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
	{
		//수정 필요
		close(this->port);
		throw bindError();
	}
	if (listen(server_socket, 15) == -1)
	{
		close(port);
		throw listenError();
	}
	fcntl(this->server_socket, F_SETFL, O_NONBLOCK);

	this->kq = kqueue();

	if (kq == -1)
	{
		close(this->port);
		throw kqueueError();
	}

	changeEvent(change_list, this->server_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
}

void Server::changeEvent(std::vector<struct kevent>& change_list, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata)
{
	struct kevent temp_event;

	EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
	this->change_list.push_back(temp_event);
}

void Server::run()
{
	int new_events;
    struct kevent* curr_event;
    while (1)
    {
        /*  apply changes and return new events(pending events) */
        new_events = kevent(kq, &change_list[0], change_list.size(), event_list, 8, NULL);
        if (new_events == -1)
        {
			close(this->port);
			throw keventError();
		}

        change_list.clear(); // clear change_list for new changes

        for (int i = 0; i < new_events; ++i)
        {
            curr_event = &event_list[i];

            /* check error event return */
            if (curr_event->flags & EV_ERROR)
            {
                if (curr_event->ident == server_socket)
                {
					close(this->port);
					throw std::runtime_error("server socket error");
				}
                else
                {
                    std::cerr << "client socket error" << std::endl;
                    disconnectClient(curr_event->ident);
                }
            }
            else if (curr_event->filter == EVFILT_READ)
            {
                if (curr_event->ident == server_socket)
                {
                    /* accept new client */
                    int client_socket;
                    if ((client_socket = accept(server_socket, NULL, NULL)) == -1)
                        throw acceptError();
                    std::cout << "accept new client: " << client_socket << std::endl;
                    fcntl(client_socket, F_SETFL, O_NONBLOCK);

                    /* add event for client socket - add read && write event */
                    changeEvent(change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
                    changeEvent(change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
                    clients[client_socket] = Client(client_socket);
                }
                else if (clients.find(curr_event->ident)!= clients.end())
                {
                    /* read data from client */
                    char buf[1024];
                    int n = recv(curr_event->ident, buf, sizeof(buf), 0);

                    if (n <= 0)
                    {
                        if (n < 0)
                            std::cerr << "client read error!" << std::endl;
                        disconnectClient(curr_event->ident);
                    }
                    else
                    {
                        buf[n] = '\0';
                        clients[curr_event->ident].addBuffer(buf);
                        std::cout << "received data from " << curr_event->ident << ": " << clients[curr_event->ident].getBuffer() << std::endl;
						parseData(clients[curr_event->ident]);
						changeEvent(change_list, curr_event->ident, EVFILT_READ, EV_DISABLE, 0, 0, curr_event->udata);
						changeEvent(change_list, curr_event->ident, EVFILT_WRITE, EV_ENABLE, 0, 0, curr_event->udata);
                    }
                }
            }
            else if (curr_event->filter == EVFILT_WRITE)
            {
                /* send data to client */
                std::map<int, Client>::iterator it = clients.find(curr_event->ident);
                if (it != clients.end())
                {
                    if (!send_data[curr_event->ident].empty())
                    {
                        int n;
						std::cout << "send data from " << curr_event->ident << ": " << this->send_data[curr_event->ident] << std::endl;
                        if ((n = send(curr_event->ident, this->send_data[curr_event->ident].c_str(),
								this->send_data[curr_event->ident].size(), 0) == -1))
                        {
                            std::cerr << "client write error!" << std::endl;
                            disconnectClient(curr_event->ident);  
                        }
                        else
                        {
							this->send_data[curr_event->ident].clear();
							changeEvent(change_list, curr_event->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, curr_event->udata);
							changeEvent(change_list, curr_event->ident, EVFILT_READ, EV_ENABLE, 0, 0, curr_event->udata);
						}
                    }
                }
            }
        }
    }
}

std::string Server::handleNick(Client& client, std::stringstream& buffer_stream)
{
	std::string name;
	std::string cur_nick;
	std::string response = "";

	//NICK 뒤에 파라미터 안 들어온 경우
	if (!(buffer_stream >> name))
		response = ERR_NONICKNAMEGIVEN(client.getNickname());

	//닉네임 설정
	else
	{
		// 닉네임 중복 체크
		if (this->clients_by_name.find(name) != this->clients_by_name.end())
		{
			response = ERR_NICKNAMEINUSE(name);
		}
		else
		{
			cur_nick = client.getNickname();
			clients_by_name.erase(cur_nick);
			client.setNickname(name);
			this->clients_by_name[name] = client;
			response = ":" + cur_nick + " NICK :" + name;
		}
	}

	return response;
}


std::string Server::handleUser(Client& client, std::stringstream& buffer_stream)
{
	std::string line;
	std::string name[4];
	int cnt = 0;

	while (cnt < 4)
	{
		if (!(buffer_stream >> line))
			return ERR_NEEDMOREPARAMS(client.getNickname(), "USER");

		name[cnt] = line;
		cnt++;
	}


	
	client.setUsername(name[0]);
	client.setHostname(name[1]);
	client.setServername(name[2]);

	if (name[3][0] == ':')
	{
		name[3] = line.substr(1);
		while (buffer_stream >> line)
		{
			std::cout << line << std::endl;
			name[3] += " " + line;
			std::cout << "name : " << name[3] << std::endl;
		}

		client.setRealname(name[3]);
	}

	//"<client> :Welcome to the <networkname> Network, <nick>[!<user>@<host>]"
	// error일 경우 해당하는 에러 메세지 담아서 보낼 것
	// /r/n 제외 msg만 보내고 나중에 /r/n 더해서 send

	std::cout << client.getUsername() << " " << client.getHostname() << " " << client.getServername() << " " << client.getRealname() << std::endl;

	return RPL_WELCOME(client.getNickname());
}

std::string Server::handlePass(Client& client, std::stringstream& buffer_stream)
{
	// 이미 register된 클라이언트인 경우
	if (client.getRegister())
		return ERR_ALREADYREGISTRED(client.getNickname());

	std::string line;
	int cnt = 0;

	// PASS 뒤에 파라미터 안 들어온 경우
	if (!(buffer_stream >> line))
		return ERR_NEEDMOREPARAMS(client.getNickname(), "PASS");
	
	// password가 다른 경우
	if (this->password != line)
		return ERR_PASSWDMISMATCH(client.getNickname());

	// 성공
	client.setRegister(true);

	return "";
}

// std::string Server::handleWho(Client& client, std::stringstream buffer_stream)
// {

// 	return "";
// }

std::string Server::handlePingpong(Client& client, std::stringstream& buffer_stream)
{
	std::string response;

	std::string ping;
	buffer_stream >> ping;

	if (ping.empty())
		response = ERR_NOORIGIN(client.getNickname());
	else
		response = RPL_PONG(client.getPrefix(), ping);
	return response;
}

std::string Server::handleJoin(Client& client, std::stringstream& buffer_stream)
{
	std::string response;

	std::string ch_name;
	std::string key;

	buffer_stream >> ch_name;
	buffer_stream >> key;

	if (ch_name.empty())
	{
		// join 이후 아무 매개변수도 들어오지 않았을 경우
		response = ERR_NEEDMOREPARAMS(client.getNickname(), "JOIN");
		return response;
	}
	if (client.getChannels().size() > CLIENT_CHANLIMIT)
	{
		// 클라이언트가 최대 채널 수에 가입했을 경우
		response = ERR_TOOMANYCHANNELS(client.getNickname(), ch_name);
		return response;
	}

	Channel *p_channel;

	if (this->channels.find(ch_name) != this->channels.end())
		p_channel = this->channels[ch_name];
	else
		p_channel = createChannel(ch_name, key, client);
	
	if (p_channel->getUsers().size() + 1 > p_channel->getUserLimit())
	{
		// 채널의 제한 인원이 꽉 찼을 경우
		response = ERR_CHANNELISFULL(client.getNickname(), ch_name);
		return response;
	}
	if ((!p_channel->getPassword().empty() && key.empty()) 
		|| (!p_channel->getPassword().empty() && key != p_channel->getPassword())
			|| (p_channel->getPassword().empty() && !key.empty()))
	{
		// channel 비밀번호가 존재하는데 request에 비밀번호가 없을 경우
		// channel 비밀번호가 존재하는데 request 비밀번호와 다를 경우
		// channel 비밀번호가 존재하지 않는데 request 비밀번호가 존재할 경우
		response = ERR_BADCHANNELKEY(client.getNickname(), ch_name);

		std::cout << response;
		return response;
	}
	if (p_channel->getInviteMode())
	{
		// invite only 채널일 경우
		response = ERR_INVITEONLYCHAN(client.getNickname(), ch_name);
		return response;
	}
	
	try
	{
		p_channel->joinClient(client);
		client.joinChannel(p_channel);
		
		std::string s_users = "";

		std::map<std::string, Client> users;
		

		for(std::map<std::string, Client>::iterator it = users.begin(); it != users.end(); it++)
		{
			s_users.append(it->first + " ");
		}

		response = makeCRLF(RPL_NAMREPLY(client.getNickname(), '=', ch_name, s_users));
		response += makeCRLF(RPL_ENDOFNAMES(client.getNickname(), ch_name));
		broadcast(ch_name, RPL_JOIN(client.getNickname(), ch_name));
	}
	catch(const std::exception& e)
	{
		// channel에서 ban 됐을 경우
		response = ERR_BANNEDFROMCHAN(client.getNickname(), ch_name);
	}
	return response;
}

std::string Server::handlePrivmsg(Client& client, std::stringstream& buffer_stream)
{
	std::string target;
	std::string msg;

	buffer_stream >> target;
	msg = buffer_stream.str();

	for (std::map<int, Client>::iterator c_it = this->clients.begin(); c_it != this->clients.end(); c_it++)
	{
		std::string name = c_it->second.getNickname();
		if (target == name)
		{
			directMsg(c_it->second, RPL_PRIVMSG(client.getNickname(), target, msg));
			break ;
		}
		std::map<std::string, Channel> c_ch = c_it->second.getChannels();
		std::map<std::string, Channel>::iterator find = c_ch.find(target);
		if (find != c_ch.end())
		{
			broadcastNotSelf(target, RPL_PRIVMSG(client.getNickname(), target, msg), client.getSocket());
			break ;
		}
	}
	// error 처리
	return "";
}

void Server::directMsg(Client& to, const std::string& msg)
{
	this->send_data[to.getSocket()] += makeCRLF(msg);
	changeEvent(change_list, to.getSocket(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
}

void Server::broadcast(std::string& channel_name, const std::string& msg)
{
	Channel *channel = this->channels[channel_name];

	std::map<std::string, Client> users = channel->getUsers();
	for(std::map<std::string, Client>::iterator u_it = users.begin(); u_it != users.end(); u_it++)
	{
		int c_socket = u_it->second.getSocket();
		this->send_data[c_socket] += makeCRLF(msg);
		changeEvent(change_list, c_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	}
}

void Server::broadcastNotSelf(std::string& channel_name, const std::string& msg, int self)
{
	Channel *channel = this->channels[channel_name];

	std::map<std::string, Client> users = channel->getUsers();
	for(std::map<std::string, Client>::iterator u_it = users.begin(); u_it != users.end(); u_it++)
	{
		int c_socket = u_it->second.getSocket();
		if (c_socket != self)
		{
			this->send_data[c_socket] += makeCRLF(msg);
			changeEvent(change_list, c_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
		}
	}
}

Channel* Server::createChannel(std::string& channel_name, std::string& key, Client& client)
{
	Channel* channel = new Channel(channel_name, key, client);
	this->channels[channel_name] = channel;
	return channel;
}

std::string Server::makeCRLF(const std::string& cmd)
{
	return cmd + "\r\n";
}

void Server::parseData(Client& client)
{
	std::string buffer = client.getBuffer();

	size_t pos = 0;

	while (1)
	{
		std::string line;

		if (buffer.find("\r\n") != std::string::npos)
		{
			pos = buffer.find("\r\n");
			line = buffer.substr(0, pos + 1);
			std::cout << "line : " << line << std::endl;
		}
		else
		{
			std::string left_line = buffer;
			client.clearBuffer();
			if (!left_line.empty())
				client.addBuffer(left_line);
			break;
		}

		std::string method;
		std::string response;
		std::stringstream buffer_stream(line);

		buffer_stream >> method;

		if (method != "CAP" && !client.getRegister())
		{
			if (method == "PASS")
			{
				response = handlePass(client, buffer_stream);
			}
			else
			{
				response = ERR_PASSWDMISMATCH(client.getNickname());
				this->send_data[client.getSocket()] += makeCRLF(response);
				client.clearBuffer();
				// 비밀번호 틀린 경우 클라이언트 접속 해제
				return ;
			}
		}
		else if (method == "NICK")
		{
			response = handleNick(client, buffer_stream);
		}
		else if (method == "USER")
		{
			response = handleUser(client, buffer_stream);
		}
		else if (method == "PING")
		{
			response = handlePingpong(client, buffer_stream);
		}
		else if (method == "JOIN")
		{
			response = handleJoin(client, buffer_stream);
		}
		else if (method == "PRIVMSG")
		{
			response = handlePrivmsg(client, buffer_stream);
		}
		// else if (method == "WHOIS" || method == "WHO")
		// {
		// 	response = handleWho()
		// }
		this->send_data[client.getSocket()] += makeCRLF(response);
		std::cout << "send data : " << response << std::endl;

		buffer = buffer.substr(pos + 2, std::string::npos);
	}
}

void Server::disconnectClient(int client_fd)
{
    close(client_fd);
    this->clients.erase(client_fd);
}

void Server::setPort(int port)
{
	this->port = port;
}

int Server::getPort() const
{
	return this->port;
}

void Server::setPassword(std::string password)
{
	this->password = password;
}

std::string Server::getPassword() const
{
	return this->password;
}

std::map<std::string, Channel*> Server::getChannels() const
{
	return this->channels;
}

std::map<int, Client> Server::getClients() const
{
	return this->clients;
}
