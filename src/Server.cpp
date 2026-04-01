
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Command.hpp"
#include "../include/Channel.hpp"
#include "../include/Utils.hpp"

Server::Server()
		:server_is_running(true) {
	initReplies();
}

Server::Server(const std::string& pass) : server_is_running(true), password(pass) {
	initReplies();
};

Server::~Server() {}

void Server::setPass(const char* pass) {
	password = pass;
}

const std::map<std::string, Channel>& Server::getChannels() const {
	return channels;
}

const std::map<int, Client>& Server::getClients() const {
	return clients;
}

Channel* Server::findChannel(const std::string& name) {
	std::string key = ircCasefold(name);
	ChannelIt it = channels.find(key);
	return it == channels.end() ? NULL : &it->second;
}
  
Client* Server::findClientByNick(const std::string& nick) {
	std::string f = ircCasefold(nick);
	for (ClientIt it = clients.begin(); it != clients.end(); ++it)
	{
		if (ircCasefold(it->second.getNick()) == f) 
		  return &it->second;
	}
	return NULL;
}

void Server::addClient(int fd, const Client& client) {
	clients[fd] = client;
}

int Server::initSocket(int port) {
	listening_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listening_fd < 0)
		return -1;
	int opt = 1;  // value to enable the option
	if (setsockopt(listening_fd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(opt)) < 0) { //to tell the operating system that your server wants to reuse the port immediately if it restarts
		return(close(listening_fd) , -1);
	}
	struct sockaddr_in server_addr;
	server_addr.sin_family		= AF_INET; //AF_INET means IPv4.
	server_addr.sin_port		= htons(port); //htons() converts host byte order to network byte order.
	server_addr.sin_addr.s_addr	= INADDR_ANY;  //INADDR_ANY means “bind to all available interfaces”
	memset(&(server_addr.sin_zero), 0, 8);
	
	if (bind(listening_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 ) {
		return(close(listening_fd) , -1);
	}
	
	if (listen(listening_fd, BACKLOG) < 0) {
		return(close(listening_fd) , -1);
	}
	return 0;
}

void Server::acceptNewConnections() {
	struct sockaddr_in	client_addr;
	socklen_t addr_len = sizeof(client_addr);
	int client_fd = accept(listening_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd < 0) {
		std::cerr << "[SERVER] " << "Accept failed on New Connection" << std::endl;
		return ;
	}
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) == -1) {// F_SETFL (command,to set the file status flags for this descriptor.) O_NONBLOCK (tells the OS that all I/O operations on this socket should be non-blocking.
		std::cerr << "[SERVER] " << "Client_fd: " << client_fd << ", could not be set to non_block" << std::endl;
		close(client_fd);							//	Non-blocking means: if you call recv() or send() and data isn’t available (or the socket can’t accept more data), the call returns immediately instead of blocking the program)
		return ;
	}
	if (clients.find(client_fd) == clients.end()) {
		clients.insert(std::pair<int, Client>(client_fd, Client(client_fd)));
	}
	else {
		std::cerr << "[SERVER] " << "Client_fd: " << client_fd << ", already exists, discarded the new connection" << std::endl;
		close(client_fd);
		return ;
	}
}

std::vector<struct pollfd> Server::buildPollFds() {
	std::vector<struct pollfd> pollfds;

	// Add listening socket
	struct pollfd server_fd;
	server_fd.fd = listening_fd;
	server_fd.events = POLLIN;
	server_fd.revents = 0;
	pollfds.push_back(server_fd);

	// Add all clients
	std::map<int, Client>::iterator it;
	for (it = clients.begin(); it != clients.end(); ++it) {
		struct pollfd pfd;
		pfd.fd = it->first;
		pfd.events = POLLIN;  // POLLIN for receiving; check for POLLOUT
		if (!it->second.buffer_out.empty()) {
			pfd.events |= POLLOUT; // monitor writability only if needed
		}
		pfd.revents = 0;
		pollfds.push_back(pfd);
	}
	return pollfds;
}

void Server::flushSendBuffer(int client_fd) {
	// 1st we find the client in the clients map
	std::map<int, Client>::iterator it = clients.find(client_fd);
	if (it == clients.end())
		return; // = client no longer exists
	Client &client = it->second;
	if (client.buffer_out.empty())
		return; // = nothing to send
	ssize_t bytes_sent = send(client_fd, client.buffer_out.c_str(), client.buffer_out.size(), 0);
	if (bytes_sent > 0) {
		// Remove sent bytes from the front of the buffer
		client.buffer_out.erase(0, bytes_sent);
	} else if (bytes_sent == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// Socket was not ready, we try again next loop
			return;
		} else {
			// Another error occurred, close the client
			std::cerr << "[SERVER] Error sending to client " << client_fd << ", closing connection" << std::endl;
			closeConnection(client_fd);
			return;
		}
	}
}

Command parseCommand(const std::string &line) {
	Command cmd;
	std::istringstream iss(line);
	std::string token;
	
	// First word would be the command
	if (!(iss >> cmd.name))
		return cmd; //Empty Command

	for (size_t i = 0; i < cmd.name.size(); ++i)
        cmd.name[i] = std::toupper(cmd.name[i]);
		
	/// Read the rest
	while (iss >> token) {
		if (token[0] == ':') {
			cmd.isColon = true;
			std::string trail = token.substr(1);
			std::string rest;
			std::getline(iss, rest);
			trail += rest;
			cmd.params.push_back(trail);
		} else {
			cmd.params.push_back(token);
		}
	}
	return cmd;
}	

void Server::handleClientMessages(int client_fd) {
	std::map<int, Client>::iterator it = clients.find(client_fd);
	if (it == clients.end())
		return;

	Client &client = it->second;
	char buffer[512];
	ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0';
		client.buffer_in += buffer;
		
		size_t pos;
		while ((pos = client.buffer_in.find('\n')) != std::string::npos) {
			std::string command = client.buffer_in.substr(0, pos);
			client.buffer_in.erase(0, pos + 1);
			// Strip trailing \r (if present from real IRC client)
			if (!command.empty() && command[command.size() - 1] == '\r')
				command.erase(command.size() - 1);
			if (command.empty())
				continue;
			// std::cout << "[DEBUG] Parsed command line: '" << command << "'" << std::endl;
			Command cmd = parseCommand(command);
			dispatch(client, cmd);
			if (clients.find(client_fd) == clients.end())
				return; // client was closed, stop immediately	
		}

	} else if (bytes_read == 0) {
		std::cerr << "[SERVER] Client " << client_fd << " disconnected" << std::endl;
		closeConnection(client_fd);
	} else {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			std::cerr << "[SERVER] Error reading from client " << client_fd << std::endl;
			closeConnection(client_fd);
		}
	}
}

void Server::closeConnection(int client_fd, const std::string &quitMessage, bool announceQuit) {
	std::map<int, Client>::iterator it = clients.find(client_fd);
	if (it != clients.end()) {
		std::cerr << "[SERVER] Closing connection with client_fd: " << client_fd << std::endl;
		Client &client = it->second;
		std::vector<std::string> emptyChannels;

		for (ChannelIt chIt = channels.begin(); chIt != channels.end(); ++chIt) {
			Channel &channel = chIt->second;
			if (channel.isMember(client_fd)) {
				if (announceQuit && client.getRegistered()) {
					std::string msg = ":" + client.getPrefix() + " QUIT :" + quitMessage;
					broadcast(channel, msg, client_fd);
				}
				channel.removeMember(client_fd);
			}
			if (channel.isInvited(client_fd))
				channel.removeInvite(client_fd);
			if (channel.getMembers().empty())
				emptyChannels.push_back(chIt->first);
		}
		for (size_t i = 0; i < emptyChannels.size(); ++i)
			channels.erase(emptyChannels[i]);
		clients.erase(it);
	}
	if (close(client_fd) == -1) {
		std::cerr << "[SERVER] Failed to close client_fd: " << client_fd << std::endl;
	}
}

void Server::stop() {
	server_is_running = false;
}

void Server::shutdown() {
	std::cerr << "[SERVER] Shutting down, closing all connections..." << std::endl;
	while (!clients.empty())
		closeConnection(clients.begin()->first, "Server shutting down", false);
	if (listening_fd >= 0) {
		close(listening_fd);
		listening_fd = -1;
	}
}

void Server::run() {
	while (server_is_running){
		std::vector<struct pollfd> pollfds = buildPollFds();
		int ret = poll(&pollfds[0], pollfds.size(), 100);
		if (ret < 0) {
			if (errno == EINTR) { // Interrupted by signal 
				continue;
			}
			std::cerr << "[SERVER] Poll failed" << std::endl;
			break;
		}
		for (size_t i = 0; i < pollfds.size(); ++i) {
			if (pollfds[i].revents & POLLIN) {
				if (pollfds[i].fd == listening_fd)
					acceptNewConnections();
				else
					handleClientMessages(pollfds[i].fd);
			}
			if (pollfds[i].revents & POLLOUT) {
				flushSendBuffer(pollfds[i].fd);
			}
			if (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
				if (clients.find(pollfds[i].fd) != clients.end()) {
					closeConnection(pollfds[i].fd);
				}
			}
		}
	}
	shutdown();
}