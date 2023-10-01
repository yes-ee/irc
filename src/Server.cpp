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

	sockaddr_in server_address;

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(this->port);

	if (bind(this->server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
	{
		// 수정 필요
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
	// 서버 소켓의 read를 큐에 등록
	changeEvent(change_list, this->server_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
}

// change_list 에 새 이벤트 추가
void Server::changeEvent(std::vector<struct kevent> &change_list, uintptr_t ident, int16_t filter, uint16_t flags, uint32_t fflags, intptr_t data, void *udata)
{
	struct kevent temp_event;

	// kevent 구조체인 temp_event를 인자들로 설정
	EV_SET(&temp_event, ident, filter, flags, fflags, data, udata);
	// 설정한 이벤트를 kevent 배열에 추가
	this->change_list.push_back(temp_event);
}

void Server::run()
{
	int new_events;
	struct kevent *curr_event;
	while (1)
	{
		/*  apply changes and return new events(pending events) */
		// change_list 에 있는 이벤트들을 kqueue에 등록
		// change_list = 큐에 등록할 이벤트들이 담겨있는 배열
		// event_list = 발생할 이벤트들이 리턴될 배열
		new_events = kevent(kq, &change_list[0], change_list.size(), event_list, 8, NULL);
		if (new_events == -1)
		{
			close(this->port);
			throw keventError();
		}

		// 큐에 다 담았으니 change_list 초기화
		change_list.clear();

		// 리턴된 이벤트를 체크
		for (int i = 0; i < new_events; ++i)
		{
			// 하나씩 돌면서 확인
			curr_event = &event_list[i];

			// 이벤트 리턴값이 error인 경우 (이벤틑 처리 과정에서 에러 발생)
			if (curr_event->flags & EV_ERROR)
			{
				// 서버에서 에러가 난 경우 -> 서버 포트 닫고, 에러 던지고 프로그램 종료
				if (curr_event->ident == server_socket)
				{
					close(this->port);
					throw std::runtime_error("server socket error");
				}
				// 클라이언트에서 에러가 난 경우 -> 해당 클라이언트 소켓 닫기 (관련된 이미 등록된 이벤트는 큐에서 삭제됨)
				else
				{
					std::cerr << "client socket error" << std::endl;
					disconnectClient(curr_event->ident);
				}
			}
			// read 가 가능한 경우
			else if (curr_event->filter == EVFILT_READ)
			{
				// 서버인 경우 (클라이언트가 새로 접속한 경우)
				if (curr_event->ident == server_socket)
				{
					/* accept new client */
					int client_socket;
					if ((client_socket = accept(server_socket, NULL, NULL)) == -1)
						throw acceptError();
					std::cout << "accept new client: " << client_socket << std::endl;
					fcntl(client_socket, F_SETFL, O_NONBLOCK);

					/* add event for client socket - add read && write event */
					// 새로 등록된 경우 클라이언트의 read와 write 이벤트 모두 등록
					changeEvent(change_list, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
					changeEvent(change_list, client_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
					// 클라이언트 목록에 추가
					clients[client_socket] = Client(client_socket);
				}
				// 이미 연결된 클라이언트의 read 가 가능한 경우
				else if (clients.find(curr_event->ident) != clients.end())
				{
					/* read data from client */
					char buf[1024];
					// 해당 클라이언트의 데이터 읽기
					int n = recv(curr_event->ident, buf, sizeof(buf), 0);

					// 에러 발생 시 클라이언트 연결 끊기
                    if (n <= 0)
                    {
                        if (n < 0)
                            std::cerr << "client read error!" << std::endl;
                        disconnectClient(curr_event->ident);
                    }
                    else
                    {
						// if (clients[curr_event->ident].getClose())
						// {
						// 	std::cout << "end : not read" << std::endl;
						// 	continue;
						// }
                        buf[n] = '\0';
                        clients[curr_event->ident].addBuffer(buf);
                        std::cout << "received data from " << curr_event->ident << ": " << clients[curr_event->ident].getBuffer() << std::endl;
						// 읽은 데이터 파싱해서 write할 데이터 클라이언트 배열의 버퍼에 넣기
						parseData(clients[curr_event->ident]);
						// 버퍼가 비어있지 않은 경우에만 write 이벤트로 전환
						if (!send_data[curr_event->ident].empty())
						{
							// read 이벤트 리턴 x -> 발생해도 큐에서 처리 x
							changeEvent(change_list, curr_event->ident, EVFILT_READ, EV_DISABLE, 0, 0, curr_event->udata);
							// write 이벤트 등록
							changeEvent(change_list, curr_event->ident, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, curr_event->udata);
						}
					}
				}
			}
			// write 가 가능한 경우
			else if (curr_event->filter == EVFILT_WRITE)
			{
				/* send data to client */
				std::map<int, Client>::iterator it = clients.find(curr_event->ident);
				if (it != clients.end())
				{ // 버퍼가 비어있는 경우 전송 x
					// 버퍼에 문자가 있으면 전송
					if (!send_data[curr_event->ident].empty())
					{
						int n;
						std::cout << "send data from " << curr_event->ident << ": " << this->send_data[curr_event->ident] << std::endl;
						if ((n = send(curr_event->ident, this->send_data[curr_event->ident].c_str(),
									  this->send_data[curr_event->ident].size(), 0) == -1))
						{
							// 전송하다 에러난 경우 연결 끊기
							std::cerr << "client write error!" << std::endl;
							disconnectClient(curr_event->ident);
						}
						// 전송 성공한 경우
						// 버퍼 비우기
						else
						{
							this->send_data[curr_event->ident].clear();
							// disconnectClient(curr_event->ident);
							if (clients[curr_event->ident].getClose())
							{
								disconnectClient(curr_event->ident);
								continue;
							}
							// write 이벤트 리턴 x -> 발생해도 큐에서 처리 x
							changeEvent(change_list, curr_event->ident, EVFILT_WRITE, EV_DISABLE, 0, 0, curr_event->udata);
							// changeEvent(change_list, curr_event->ident, EVFILT_WRITE, EV_DELETE, 0, 0, curr_event->udata);
							// read 이벤트 등록
							changeEvent(change_list, curr_event->ident, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, curr_event->udata);
						}
					}
				}
			}
		}
	}
}

std::string Server::handleNick(Client &client, std::stringstream &buffer_stream)
{
	std::string name;
	std::string cur_nick;
	std::string response = "";

	// NICK 뒤에 파라미터 안 들어온 경우
	if (!(buffer_stream >> name))
		response = ERR_NONICKNAMEGIVEN(client.getNickname());

	// 닉네임 설정
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

std::string Server::handleUser(Client &client, std::stringstream &buffer_stream)
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

std::string Server::handlePass(Client &client, std::stringstream &buffer_stream)
{
	// 이미 register된 클라이언트인 경우
	if (client.getRegister())
		return ERR_ALREADYREGISTRED(client.getNickname());

	int cnt = 0;
	std::string line;

	buffer_stream >> line;

	// PASS 뒤에 파라미터 안 들어온 경우
	if (line.empty())
	{
		client.setClose(true);
		return ERR_NEEDMOREPARAMS(client.getNickname(), "PASS");
	}

	// password가 다른 경우
	if (this->password != line)
	{
		client.setClose(true);
		return ERR_PASSWDMISMATCH(client.getNickname());
	}

	// 성공
	client.setRegister(true);

	return "";
}

std::string Server::handleQuit(Client &client, std::stringstream &buffer_stream)
{
	std::string line;
	std::string message;

	buffer_stream >> line;

	message = line.substr(1);
	std::cout << message << std::endl;

	while (1)
	{
		if (!(buffer_stream >> line))
			break;
		message += " " + line;
	}

	return message;
}

std::string Server::handleWho(Client &client, std::stringstream &buffer_stream)
{
	std::string name;
	std::string ch_name;
	std::string option = "H";
	std::string reply;
	std::string response = "";

	buffer_stream >> name;

	if (!name.empty())
	{
		// channel name
		if (name[0] == '#')
		{
			for (std::map<std::string, Channel *>::iterator m_it = this->channels.begin(); m_it != this->channels.end(); m_it++)
			{
				ch_name = m_it->second->getName();
				if (name == ch_name)
				{
					std::map<std::string, Client> users = m_it->second->getUsers();

					for (std::map<std::string, Client>::iterator u_it = users.begin(); u_it != users.end(); u_it++)
					{
						option = "H";
						if (m_it->second->isOperator(u_it->second))
							option += "@";

						reply = RPL_WHOREPLY(client.getNickname(), ch_name, u_it->second.getUsername(), u_it->second.getHostname(),
											 u_it->second.getServername(), u_it->second.getNickname(), option, u_it->second.getRealname());
						response += makeCRLF(reply);
					}
				}
			}
		}
		// user name
		else
		{
			for (std::map<int, Client>::iterator u_it = this->clients.begin(); u_it != this->clients.end(); u_it++)
			{
				if (name == u_it->second.getNickname())
				{
					if (u_it->second.getChannels().empty())
					{
						ch_name = "*";
					}
					else
					{
						ch_name = u_it->second.getChannels().begin()->first;
						Channel *ch = this->channels[ch_name];
						if (ch->isOperator(u_it->second))
							option += "@";
					}

					reply = RPL_WHOREPLY(client.getNickname(), ch_name, u_it->second.getUsername(), u_it->second.getHostname(),
										 u_it->second.getServername(), u_it->second.getNickname(), option, u_it->second.getRealname());
					response += makeCRLF(reply);
				}
			}
		}
	}

	response += RPL_ENDOFWHO(client.getNickname(), name);

	return response;
}

std::string Server::handlePingpong(Client &client, std::stringstream &buffer_stream)
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

std::string Server::handleJoin(Client &client, std::stringstream &buffer_stream)
{
	std::string response;

	std::string ch_name;
	std::string key;

	buffer_stream >> ch_name;
	buffer_stream >> key;

	if (ch_name.empty())
	{
		// join 이후 아무 매개변수도 들어오지 않았을 경우
		response += ERR_NEEDMOREPARAMS(client.getNickname(), "JOIN");
		return response;
	}
	if (client.getChannels().size() > CLIENT_CHANLIMIT)
	{
		// 클라이언트가 최대 채널 수에 가입했을 경우
		response += ERR_TOOMANYCHANNELS(client.getNickname(), ch_name);
		return response;
	}

	std::vector<std::string> channels;

	std::stringstream channel_stream(ch_name);
	std::string channel;
	while (std::getline(channel_stream, channel, ','))
	{
		channel.erase(std::remove(channel.begin(), channel.end(), '\r'));
		channel.erase(std::remove(channel.begin(), channel.end(), '\n'));
		channels.push_back(channel);
	}

	std::vector<std::string> keys;

	std::stringstream key_stream(key);
	std::string channel_key;
	while (std::getline(key_stream, channel_key, ','))
	{
		channel_key.erase(std::remove(channel_key.begin(), channel_key.end(), '\r'));
		channel_key.erase(std::remove(channel_key.begin(), channel_key.end(), '\n'));
		keys.push_back(channel_key);
	}

	int i = 0;

	for (std::vector<std::string>::iterator it = channels.begin(); it != channels.end(); it++)
	{
		if (i < keys.size())
			key = keys[i];
		else
			key = "";
		response += clientJoinChannel(client, *it, key);
		i++;
	}
	return response;
}

std::string Server::clientJoinChannel(Client &client, std::string &ch_name, std::string &key)
{
	std::string response;

	Channel *p_channel;

	if (this->channels.find(ch_name) != this->channels.end())
		p_channel = this->channels[ch_name];
	else
		p_channel = createChannel(ch_name, key, client);

	if (p_channel->getUsers().size() + 1 > p_channel->getUserLimit())
	{
		// 채널의 제한 인원이 꽉 찼을 경우
		response += ERR_CHANNELISFULL(client.getNickname(), ch_name);
		return response;
	}
	if ((!p_channel->getPassword().empty() && key.empty()) || (!p_channel->getPassword().empty() && key != p_channel->getPassword()) || (p_channel->getPassword().empty() && !key.empty()))
	{
		// channel 비밀번호가 존재하는데 request에 비밀번호가 없을 경우
		// channel 비밀번호가 존재하는데 request 비밀번호와 다를 경우
		// channel 비밀번호가 존재하지 않는데 request 비밀번호가 존재할 경우
		response += ERR_BADCHANNELKEY(client.getNickname(), ch_name);

		return response;
	}
	if (p_channel->getInviteMode())
	{
		// invite only 채널일 경우
		response += ERR_INVITEONLYCHAN(client.getNickname(), ch_name);
		return response;
	}

	try
	{
		if (client.getNickname() != p_channel->getOwner().getNickname())
			p_channel->joinClient(client);
		client.joinChannel(p_channel);

		std::string s_users = "";

		std::map<std::string, Client> users = p_channel->getUsers();

		for (std::map<std::string, Client>::iterator it = users.begin(); it != users.end(); it++)
		{
			if (it->first == p_channel->getOwner().getNickname())
				s_users.append("@");
			s_users.append(it->first + " ");
		}
		broadcast(ch_name, RPL_JOIN(client.getPrefix(), ch_name));
		if (!p_channel->getTopic().empty())
		{
			response += makeCRLF(RPL_TOPIC(client.getUsername(), ch_name, p_channel->getTopic()));
			response += makeCRLF(RPL_TOPICWHOTIME(client.getUsername(), ch_name, p_channel->getTopicUser(), p_channel->getTopicTime()));
		}
		response += makeCRLF(RPL_NAMREPLY(client.getNickname(), '=', ch_name, s_users));
		response += makeCRLF(RPL_ENDOFNAMES(client.getNickname(), ch_name));
	}
	catch (const std::exception &e)
	{
		// channel에서 ban 됐을 경우
		response += makeCRLF(ERR_BANNEDFROMCHAN(client.getNickname(), ch_name));
	}
	return response;
}

std::string Server::handlePrivmsg(Client &client, std::stringstream &buffer_stream)
{
	std::string response;
	std::string target;
	std::string msg;

	buffer_stream >> target;
	std::getline(buffer_stream, msg);
	msg.erase(std::remove(msg.begin(), msg.end(), '\r'));
	msg.erase(std::remove(msg.begin(), msg.end(), '\n'));
	if (target[0] == '#')
	{
		response += msgToServer(client, target, msg);
	}
	else
	{
		response += msgToUser(client, target, msg);
	}
	// error 처리
	return response;
}

std::string Server::msgToServer(Client &client, std::string &target, std::string &msg)
{
	std::string response;

	if (this->channels.find(target) == this->channels.end())
	{
		response += makeCRLF(ERR_NOSUCHCHANNEL(client.getNickname(), target));
	}
	else
	{
		Channel *channel = this->channels[target];
		std::map<std::string, Client> users = channel->getUsers();

		if ((users.find(client.getNickname()) == users.end()) && (channel->getModes().find('n') != channel->getModes().end()))
		{
			// 해당 사용자가 해당 채널에도 없고 n 옵션이 설정되어 있을 경우
			response += makeCRLF(ERR_CANNOTSENDTOCHAN(client.getNickname(), target));
		}
		else
			broadcastNotSelf(target, RPL_PRIVMSG(client.getPrefix(), target, msg), client.getSocket());
	}
	return response;
}

std::string Server::msgToUser(Client &client, std::string &target, std::string &msg)
{
	for (std::map<int, Client>::iterator c_it = this->clients.begin(); c_it != this->clients.end(); c_it++)
	{
		std::string name = c_it->second.getNickname();
		if (target == name)
		{
			directMsg(c_it->second, RPL_PRIVMSG(client.getPrefix(), target, msg));
			return "";
		}
	}
	return makeCRLF(ERR_NOSUCHNICK(client.getNickname(), target));
}

void Server::directMsg(Client &to, const std::string &msg)
{
	this->send_data[to.getSocket()] += makeCRLF(msg);
	changeEvent(change_list, to.getSocket(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
}

void Server::broadcast(std::string &channel_name, const std::string &msg)
{
	Channel *channel = this->channels[channel_name];

	std::map<std::string, Client> users = channel->getUsers();
	for (std::map<std::string, Client>::iterator u_it = users.begin(); u_it != users.end(); u_it++)
	{
		int c_socket = u_it->second.getSocket();
		this->send_data[c_socket] += makeCRLF(msg);
		changeEvent(change_list, c_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
	}
}

void Server::broadcastNotSelf(std::string &channel_name, const std::string &msg, int self)
{
	Channel *channel = this->channels[channel_name];

	std::map<std::string, Client> users = channel->getUsers();
	for (std::map<std::string, Client>::iterator u_it = users.begin(); u_it != users.end(); u_it++)
	{
		int c_socket = u_it->second.getSocket();
		if (c_socket != self)
		{
			this->send_data[c_socket] += makeCRLF(msg);
			changeEvent(change_list, c_socket, EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);
		}
	}
}

Channel *Server::createChannel(std::string &channel_name, std::string &key, Client &client)
{
	Channel *channel = new Channel(channel_name, key, client);
	this->channels[channel_name] = channel;
	return channel;
}

std::string Server::makeCRLF(const std::string &cmd)
{
	return cmd + "\r\n";
}

std::string Server::handleList(Client &client, std::stringstream &buffer_stream)
{
	std::string response;

	std::string ch_name;
	buffer_stream >> ch_name;
	if (ch_name.empty())
	{
		response += makeCRLF(RPL_LISTSTART(client.getNickname()));
		if (this->channels.size() > 0)
		{
			for (std::map<std::string, Channel *>::iterator it = this->channels.begin(); it != this->channels.end(); it++)
			{
				response += makeCRLF(RPL_LIST(client.getNickname(), it->first, std::to_string(it->second->getUsers().size()), "[" + it->second->getModeString() + "]", it->second->getTopic()));
			}
		}
		response += makeCRLF(RPL_LISTEND(client.getNickname()));
		return response;
	}
	response += makeCRLF(RPL_LISTSTART(client.getNickname()));
	Channel *ch_po = this->channels[ch_name];
	if (ch_po)
		response += makeCRLF(RPL_LIST(client.getNickname(), ch_name, std::to_string(ch_po->getUsers().size()), "[" + ch_po->getModeString() + "]", ch_po->getTopic()));
	response += makeCRLF(RPL_LISTEND(client.getNickname()));
	return response;
}

std::string Server::handleTopic(Client &client, std::stringstream &buffer_stream)
{
	std::string response;
	std::string channel;
	std::string topic;

	buffer_stream >> channel;
	buffer_stream >> topic;
	if (channel.empty())
	{
		// topic 명령어만 들어올 경우 461
		response += makeCRLF(ERR_NEEDMOREPARAMS(client.getNickname(), "TOPIC"));
		return response;
	}
	if (this->channels.find(channel) == this->channels.end())
	{
		// 없는 채널일 경우 403
		response += makeCRLF(ERR_NOSUCHCHANNEL(client.getNickname(), channel));
		return response;
	}

	Channel *ch_po = this->channels[channel];

	if (topic.empty())
	{
		// topic 없는 경우
		if (ch_po->getTopic().empty())
		{
			response += makeCRLF(RPL_NOTOPIC(client.getNickname(), channel));
			return response;
		}
		// 그냥 조회만 할 때 채널 있는 경우 332, 333
		response += makeCRLF(RPL_TOPIC(client.getNickname(), channel, ch_po->getTopic()));
		response += makeCRLF(RPL_TOPICWHOTIME(client.getNickname(), channel, ch_po->getTopicUser(), ch_po->getTopicTime()));
		return response;
	}
	std::map<std::string, Client> users = ch_po->getUsers();
	if (users.find(client.getNickname()) == users.end())
	{
		// 채널에 유저가 없을 때 변경하려고 하면 442 error
		response += makeCRLF(ERR_NOTONCHANNEL(client.getNickname(), channel));
		return response;
	}
	if (!ch_po->isOperator(client) && (ch_po->getModes().find('t') != ch_po->getModes().end()))
	{
		// 권한 없는 유저가 변경하려고 하면 482 error
		response += makeCRLF(ERR_CHANOPRIVSNEEDED(client.getNickname(), channel));
		return response;
	}
	ch_po->setTopic(client, topic);
	broadcast(channel, makeCRLF(RPL_MY_TOPIC(client.getPrefix(), channel, topic)));
	return "";
}

std::string Server::handlePart(Client &client, std::stringstream &buffer_stream)
{
	std::string response;
	std::string channel_line;

	buffer_stream >> channel_line;

	std::vector<std::string> v_channels;

	std::stringstream new_stream(channel_line);
	std::string channel;
	while (std::getline(new_stream, channel, ','))
	{
		channel.erase(std::remove(channel.begin(), channel.end(), '\r'));
		channel.erase(std::remove(channel.begin(), channel.end(), '\n'));
		v_channels.push_back(channel);
	}

	for (std::vector<std::string>::iterator it = v_channels.begin(); it != v_channels.end(); it++)
	{
		if (this->channels.find(*it) == this->channels.end())
		{
			// 없는 채널일 경우 403
			response += makeCRLF(ERR_NOSUCHCHANNEL(client.getNickname(), *it));
			return response;
		}
		Channel *ch_po = this->channels[*it];

		std::map<std::string, Client> users = ch_po->getUsers();
		if (users.find(client.getNickname()) == users.end())
		{
			// 채널에 유저가 없을 때 나가려고 하면 442 error
			response += makeCRLF(ERR_NOTONCHANNEL(client.getNickname(), *it));
			return response;
		}
		clientLeaveChannel(client, ch_po);
	}
	return response;
}

std::string Server::handleInvite(Client &client, std::stringstream &buffer_stream)
{
	std::string response;
	std::string nickname;
	std::string ch_name;

	buffer_stream >> nickname;
	buffer_stream >> ch_name;
	
	if (this->channels.find(ch_name) == this->channels.end())
	{
		// channel이 존재하지 않을 경우
		return makeCRLF(ERR_NOSUCHCHANNEL(client.getNickname(), ch_name));
	}
	if (this->clients_by_name.find(nickname) == this->clients_by_name.end())
	{
		// user가 존재하지 않을 경우
		return makeCRLF(ERR_NOSUCHNICK(client.getNickname(), nickname));
	}
	Channel *channel = this->channels[ch_name];
	std::map<std::string, Client> users = channel->getUsers();
	if (users.find(client.getNickname()) == users.end())
	{
		//채널에 없는 유저가 보냈을 경우
		return makeCRLF(ERR_NOTONCHANNEL(client.getNickname(), ch_name));
	}
	if (users.find(nickname) != users.end())
	{
		// 이미 채널에 있는 유저일 경우
		return makeCRLF(ERR_USERONCHANNEL(client.getNickname(), nickname, ch_name));
	}
	if (!channel->isOperator(client))
	{
		// operator 아닐 경우
		return makeCRLF(ERR_CHANOPRIVSNEEDED(client.getNickname(), ch_name));
	}
	Client to = this->clients_by_name[nickname];
	response = makeCRLF(RPL_INVITING(client.getNickname(), nickname, ch_name));
	directMsg(to, RPL_INVITE(client.getPrefix(), nickname, ch_name));
	return response;
}

void Server::clientLeaveChannel(Client &client, Channel *channel)
{
	std::string ch_name = channel->getName();
	broadcast(ch_name, makeCRLF(RPL_PART(client.getPrefix(), ch_name)));
	client.leaveChannel(channel);
	channel->deleteClient(client.getNickname());
	if (channel->getUsers().size() == 0)
	{
		this->channels.erase(ch_name);
		delete channel;
		channel = 0;
	}
}

void Server::parseData(Client &client)
{
	std::string buffer = client.getBuffer();
	std::cout << buffer << std::endl;

	size_t pos = 0;

	while (1)
	{
		if (client.getClose())
			break;
		
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
		std::string pre_method;
		std::string message;
		std::string response;
		std::stringstream buffer_stream(line);
		std::stringstream pre_stream;

		buffer_stream >> method;

		if (!client.getRegister())
		{
			pre_stream.str(client.getPreCmd());
			pre_stream >> pre_method;
		}

		if (method != "CAP" && !client.getRegister())
		{
			if (method == "PASS")
			{
				// 다음 버퍼까지 확인해서 마지막 pass일 때 인증 과정 수행
				// method 이후부터 저장되어 있는 pre_stream 생성
				client.setPreCmd(line);
				this->send_data[client.getSocket()] += makeCRLF(response);
				buffer = buffer.substr(pos + 2);
				continue;
			}
			// 마지막 pass일 때 인증 과정 수행
			else if (pre_method == "PASS")
			{
				std::string tmp("");
				client.setPreCmd(tmp);
				buffer.insert(0, makeCRLF(line));
				response = handlePass(client, pre_stream);
			}
			else
			{
				response = ERR_PASSWDMISMATCH(client.getNickname());
				this->send_data[client.getSocket()] = makeCRLF(response);
				client.clearBuffer();
				client.setClose(true);
				return;
			}
		}
		else if (method == "PASS")
		{
			response = handlePass(client, buffer_stream);
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
		else if (method == "QUIT")
		{
			// quit message 받아옴
			message = handleQuit(client, buffer_stream);

			response = ERR_QUIT(client.getPrefix(), message);
			this->send_data[client.getSocket()] += makeCRLF(response);

			// create response message
			response = RPL_QUIT(client.getPrefix(), message);

			// broadcast response() in channel
			std::map<std::string, Channel> channels = client.getChannels();

			// 들어가있는 모든 채널에 브로드캐스팅
			for (std::map<std::string, Channel>::iterator m_it = channels.begin(); m_it != channels.end(); m_it++)
			{
				std::string ch_name = m_it->second.getName();
				this->broadcastNotSelf(ch_name, response, client.getSocket());
			}

			client.setClose(true);
			break;
		}
		else if (method == "JOIN")
		{
			response = handleJoin(client, buffer_stream);
		}
		else if (method == "PRIVMSG")
		{
			response = handlePrivmsg(client, buffer_stream);
		}
		else if (method == "LIST")
		{
			response = handleList(client, buffer_stream);
		}
		else if (method == "WHO")
		{
			response = handleWho(client, buffer_stream);
		}
		else if (method == "TOPIC")
		{
			response = handleTopic(client, buffer_stream);
		}
		else if (method == "PART")
		{
			response = handlePart(client, buffer_stream);
		}
		else if (method == "INVITE")
		{
			response = handleInvite(client, buffer_stream);
		}
		this->send_data[client.getSocket()] += makeCRLF(response);
		std::cout << "send data : " << response << std::endl;

		buffer = buffer.substr(pos + 2, std::string::npos);
	}
}

void Server::disconnectClient(int client_fd)
{
	std::string ch_name;
	std::string nickname = clients[client_fd].getNickname();
	std::map<std::string, Channel> channels = this->clients[client_fd].getChannels();

	// 들어가있는 모든 채널에서 삭제
	for (std::map<std::string, Channel>::iterator m_it = channels.begin(); m_it != channels.end(); m_it++)
	{
		ch_name = m_it->second.getName();
		std::cout << "channel :" << ch_name << " users : " << m_it->second.getUsers().size() << std::endl;
		this->channels[ch_name]->deleteClient(nickname);
	}

	this->clients_by_name.erase(nickname);
	this->send_data.erase(client_fd);
	this->clients.erase(client_fd);
	std::cout << "close client" << std::endl;
	close(client_fd);

	for (std::map<std::string, Channel *>::iterator m_it = this->channels.begin(); m_it != this->channels.end(); m_it++)
	{
		ch_name = m_it->second->getName();
		std::cout << "channel :" << ch_name << std::endl;
		std::cout << this->channels[ch_name]->getUsers().size() << std::endl;
	}
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

std::map<std::string, Channel *> Server::getChannels() const
{
	return this->channels;
}

std::map<int, Client> Server::getClients() const
{
	return this->clients;
}