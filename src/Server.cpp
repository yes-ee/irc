#include "../inc/Server.hpp"

Server::Server()
{

}

Server::~Server()
{

}

Server::Server(int port, int password) : port(port), password(password)
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
                    }
                }
            }
            else if (curr_event->filter == EVFILT_WRITE)
            {
                /* send data to client */
                std::map<int, Client>::iterator it = clients.find(curr_event->ident);
                if (it != clients.end())
                {
                    if (clients[curr_event->ident].getBuffer() != "")
                    {
                        int n;
                        if ((n = write(curr_event->ident, clients[curr_event->ident].getBuffer().c_str(),
                                        clients[curr_event->ident].getBuffer().size()) == -1))
                        {
                            std::cerr << "client write error!" << std::endl;
                            disconnectClient(curr_event->ident);  
                        }
                        else
                            clients[curr_event->ident].clearBuffer();
                    }
                }
            }
        }
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

std::map<int, Client> Server::getClients() const
{
	return this->clients;
}
