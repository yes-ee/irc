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
						
                        if ((n = send(curr_event->ident, this->send_data[curr_event->ident].c_str(),
								this->send_data[curr_event->ident].size(), 0) == -1))
                        {
                            std::cerr << "client write error!" << std::endl;
                            disconnectClient(curr_event->ident);  
                        }
                        else
                        {
							//clients[curr_event->ident].clearBuffer();
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

std::string Server::handleUser(Client& client, std::string& cmd, std::stringstream& buffer_stream)
{
	std::string line;

	buffer_stream >> line;
	client.setUsername(line);
	buffer_stream >> line;
	client.setHostname(line);
	buffer_stream >> line;
	this->servername = line;
	line = buffer_stream.str();

	// : 로 realname 인식할 것
	if (line.length() > 0 && line[0] == ':')
	{
		std::string realname = line.substr(1, std::string::npos);
		client.setRealname(realname);
		std::cout << "realname : " << client.getRealname();
	}
	//"<client> :Welcome to the <networkname> Network, <nick>[!<user>@<host>]"
	// error일 경우 해당하는 에러 메세지 담아서 보낼 것
	// /r/n 제외 msg만 보내고 나중에 /r/n 더해서 send
	return "Welcome to the Internet Relay Network " + client.getNickname() + "!" + client.getUsername() + "@" + client.getHostname();
}

std::string Server::handlePass(Client& client, std::string& cmd)
{
	// 이미 register된 클라이언트인 경우
	if (client.getRegister())
		return ":Unauthorized command (already registered)";

	std::stringstream buffer_stream(cmd);
	std::string line;
	int cnt = 0;

	// PASS 뒤에 파라미터 안 들어온 경우
	if (buffer_stream >> line)
		return "PASS :Not enough parameters";
	
	// password가 다른 경우
	if (this->password != line)
		return ":Password incorrect";

	// 성공
	client.setRegister(true);

	// NICK, USER 받은 후 보내기
	// return "Welcome to the Internet Relay Network " + client.getNickname();
	return "";
}

std::string Server::makeSendData(Client& client, std::string& cmd)
{
	std::string status_code = "001";
	return status_code + " :" + this->servername + " " + cmd + "\r\n";
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

		std::string response;

		std::stringstream buffer_stream(line);

		std::string method;

		buffer_stream >> method;

		if (method == "NICK")
		{
			// 닉네임 유효성 검사
			std::string nickname;
			buffer_stream >> nickname;
			client.setNickname(nickname);
			response = "";
		}
		else if (method == "CAP")
		{
			std::string ls;
			buffer_stream >> ls;
			if (ls == "LS")
				response = "";
		}
		else if (method == "USER")
		{
			response = handleUser(client, line, buffer_stream);
			
		}
		this->send_data[client.getSocket()] = makeSendData(client, response);

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

void Server::addChannel(std::string& channel)
{
	this->channels[channel] = Channel(channel);
}

std::map<std::string, Channel> Server::getChannels() const
{
	return this->channels;
}

std::map<int, Client> Server::getClients() const
{
	return this->clients;
}
