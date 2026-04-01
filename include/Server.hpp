#ifndef SERVER_HPP
# define SERVER_HPP

#include <map>
#include <iostream>
#include <vector>
#include <sys/socket.h> // for socket, bind, listen, accept
#include <netinet/in.h> // for sockadd_in
#include <fcntl.h> // for fcntl to set fd to non blocking mode
#include <unistd.h> // for close
#include <iomanip>
#include <cstring> // for memset
#include <sstream>
#include <cerrno> //for errno
#include <poll.h>
#include <sys/types.h>

#include "NumericReplies.hpp"
#include <map>

# define BACKLOG 10

class Client;
class Command;
class Channel;

class Server {
	private:
		int 		listening_fd;
		bool		server_is_running;
		std::string	password;
		
		std::map<int, Client> 			clients;
		std::map<std::string, Channel>	channels;
		std::map<int, std::string> 		replies;
		
		void initReplies();

		void acceptNewConnections();
		std::vector<struct pollfd> buildPollFds();
		void flushSendBuffer(int client_fd);
		void closeConnection(int client_fd, const std::string &quitMessage = "Client Quit", bool announceQuit = false);
		void handleClientMessages(int fd);
		void sendToClient(Client &client, const std::string &msg);
		
	public:
		Server();
		Server(const std::string& pass);
		~Server();
		int initSocket(int port);
		void run();
		void setPass(const char* pass);
		void stop();
		void shutdown();
		
		// ==================== Accessors ===================
		
		typedef std::map<std::string, Channel>::iterator ChannelIt;
		typedef std::map<std::string, Channel>::const_iterator ConstChannelIt;
		typedef std::map<int, Client>::iterator ClientIt;
		typedef std::map<int, Client>::const_iterator ConstClientIt;

		const std::map<std::string, Channel>& getChannels() const;
		Channel* findChannel(const std::string& name); 
		Client* findClientByNick(const std::string& nick);
		const std::map<int, Client>& getClients() const;
		void addClient(int fd, const Client& client);

		
		// =============== Core functionality ===============
		void dispatch(Client &client, const Command& cmd);
		void handleCommand(Client &client, const Command& cmd);
		void sendNumericReply(Client& client, int code, const std::string &extra = "", const std::string &arg = "");
		void broadcast(Channel &chan, const std::string& msg, int senderFd);

		// ================= AUTHENTICATION =================
		void handleRegistration(Client &client, const Command &cmd);
		void finishRegistration(Client &client);
		void validatePass(Client &client, const std::string &param);
		void validateNick(Client &client, const std::string &param);
		void announceNickChange(Client &client, const std::string &oldNick);
		bool nickInUse(const std::string &nick);
		void validateUser(Client &client, const std::vector<std::string> &params);

		// ==================== CHANNELS ====================
		void handleJoin(Client &client, const Command& cmd);
		void handlePart(Client &client, const Command& cmd);
		void sendChannelTopic(Client &client, Channel &channel);
		void sendChannelNameList(Client &client, Channel &channel);
		void partAllChannels(Client &client);

		// ==================== MESSAGES ====================
		void handlePrivMsg(Client &client, const Command& cmd);
		void messageToChannel(Client &client, const std::string &target, const std::string &msg);
		void messageToUser(Client &client, const std::string &targetNick, const std::string &msg);
		void handleNotice(Client &client, const Command& cmd);
		void noticeToChannel(Client &client, const std::string &target, const std::string &msg);
		void noticeToUser(Client &client, const std::string &targetNick, const std::string &msg);

		// ==================== OPERATOR ====================
		void handleKick(Client &client, const Command& cmd);
		void handleMode(Client &client, const Command& cmd);
		void handleTopic(Client &client, const Command& cmd);
		void handleInvite(Client &client, const Command& cmd);
		void changeChannelMode(Client &client, Channel &chan, const Command& cmd);
		
		bool modeK(Client &client, Channel &chan, const Command& cmd, bool mode, size_t& index);
		bool modeO(Client &client, Channel &chan, const Command& cmd, bool mode, size_t& index);
		bool modeL(Client &client, Channel &chan, const Command& cmd, bool mode, size_t& index);

		void handleCap(Client &client, const Command& cmd);
		void handlePing(Client &client, const Command& cmd);
		void handleQuit(Client &client, const Command& cmd);
};		

#endif