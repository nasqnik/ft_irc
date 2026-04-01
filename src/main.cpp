#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include <iostream>
#include <csignal> 

#include <sstream> // for int convertion on is_valid_port

static Server* g_server = 0;

bool is_valid_port(char *arg, int &port){
	std::istringstream iss(arg);
	unsigned int value= 0;
	iss >> value;
	if (iss.fail() || !iss.eof())
		return (false);
	if (value < 1024 || value > 65535)
		return false;
	port = value;
	return true;
}

void signalHandler(int signum) {
	if (g_server) {
		std::cerr << "\n[SERVER] Signal " << signum << " received, shutting down..." << std::endl;
		g_server->stop();  // to set server_is_running = false
	}
}

bool valid_input(int ac, char** av, int &port){
	if (ac != 3) {
		std::cerr<<"[SERVER] Error: Use is: "<<std::endl<<"./ircserv <port> <password>"<<std::endl;
		return(false);
	}
	if (!is_valid_port(av[1], port)) {
		std::cerr<<"[SERVER] Error: port should be only numerical and between 1024 and 65535"<<std::endl;
		return(false);
	}
	std::string pass = av[2];
	if (pass.empty() || pass.find_first_not_of(' ') == std::string::npos) {
		std::cerr<<"[SERVER] Invalid password: cannot be empty or only spaces" <<std::endl;
		return(false);
	}
	return true;
}

int main(int ac, char **av){
	int port = 0;
	if (!valid_input(ac, av, port)) {
		return 1;
	}
	Server server;
	g_server = &server;
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGPIPE, SIG_IGN);
	if (server.initSocket(port) == -1)
		return(std::cerr << "[SERVER] Error setting up listening socket" << std::endl, 1);
	server.setPass(av[2]);
	server.run();

	return 0;
}
