#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Command.hpp"
#include "../include/Channel.hpp"

#include <sstream>

void Server::initReplies() {
	replies[RPL_WELCOME]			= "Welcome to the Internet Relay Network ";
	replies[RPL_NOTOPIC]			= "No topic is set";
	replies[RPL_TOPIC]				= "";
	replies[RPL_NAMREPLY]			= ""; 
	replies[RPL_ENDOFNAMES]			= "End of /NAMES list";
	replies[ERR_NOSUCHCHANNEL]		= "No such channel";
	replies[ERR_NONICKNAMEGIVEN]	= "No nickname given";
	replies[ERR_ERRONEUSNICKNAME]	= "Erroneous nickname";
	replies[ERR_NICKNAMEINUSE]		= "Nickname is already in use";
	replies[ERR_NOTREGISTERED]	 	= "You have not registered";
	replies[ERR_NEEDMOREPARAMS]		= "Not enough parameters";
	replies[ERR_ALREADYREGISTRED]	= "Unauthorized command (already registered)";
	replies[ERR_PASSWDMISMATCH]		= "Password incorrect";
	replies[ERR_BADCHANMASK]		= "Bad Channel Mask";
	replies[ERR_NOTEXTTOSEND]		= "No text to send";
	replies[ERR_CANNOTSENDTOCHAN]	= "Cannot send to channel";
	replies[ERR_NOSUCHNICK]			= "No such nick/channel";
	replies[ERR_NORECIPIENT]		= "No recipient given ";
	replies[ERR_NOTONCHANNEL]		= "You're not on that channel";
	replies[ERR_CHANOPRIVSNEEDED]	= "You're not channel operator";
	replies[ERR_USERNOTINCHANNEL]	= "They aren't on that channel";
	replies[RPL_CHANNELMODEIS]		= "";
	replies[ERR_UNKNOWNMODE]		= "is unknown mode char to me";
	replies[ERR_USERONCHANNEL]	 	= "is already on channel";
	replies[RPL_INVITING]			= "";
	replies[ERR_INVITEONLYCHAN] 	= "Cannot join channel (+i)";
	replies[ERR_CHANNELISFULL]		= "Cannot join channel (+l)";
	replies[ERR_BADCHANNELKEY]		= "Cannot join channel (+k)";
}

void Server::dispatch(Client &client, const Command& cmd) {

	std::cout << "[Incoming request] " << cmd.name;
	for (size_t i = 0; i < cmd.params.size(); i++) {
		std::cout << " [" << cmd.params[i] << "]";
	}
	std::cout << std::endl;
   
	if (cmd.name == "CAP")
		return handleCap(client, cmd);
	if (cmd.name == "PING")
		return handlePing(client, cmd);
	if (cmd.name == "QUIT")
		return handleQuit(client, cmd);
	else if (!client.getRegistered())
		handleRegistration(client, cmd);
	else
		handleCommand(client, cmd);
}

void Server::handleCommand(Client &client, const Command& cmd) {
	
	if (cmd.name == "NICK")
		validateNick(client, cmd.params.size() > 0 ? cmd.params[0] : "");
	else if (cmd.name == "PING")
		handlePing(client, cmd);
	else if (cmd.name == "JOIN")
		handleJoin(client, cmd);
	else if (cmd.name == "USER")
		sendNumericReply(client, ERR_ALREADYREGISTRED);
	else if (cmd.name == "PASS")
		sendNumericReply(client, ERR_ALREADYREGISTRED);
	else if (cmd.name == "PRIVMSG")
		handlePrivMsg(client, cmd);
	else if (cmd.name == "NOTICE")
		handleNotice(client, cmd);
	else if (cmd.name == "PART")
		handlePart(client, cmd);
	else if (cmd.name == "KICK")
		handleKick(client, cmd);
	else if (cmd.name == "MODE")
		handleMode(client, cmd);
	else if (cmd.name == "TOPIC")
		handleTopic(client, cmd);
	else if (cmd.name == "INVITE")
		handleInvite(client, cmd);
}
 
void Server::sendNumericReply(Client& client, int code, const std::string &extra, const std::string &arg)
{
    // Format numeric code as 3 digits
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(3) << code;
    std::string codeStr = oss.str();

    std::string target = client.getNickOrStar();
    std::string reply = ":localhost " + codeStr + " " + target;

    // Argument for replies like 401, 403, 433, 473, 475, 471
    if (!arg.empty())
        reply += " " + arg;

    // Message: either extra provided or default from replies table
    std::string msg = extra.empty() ? replies[code] : extra;
    if (!msg.empty())
        reply += " :" + msg;


    sendToClient(client, reply);
    std::cout << "[Reply] " << reply << std::endl;
}

void Server::sendToClient(Client &client, const std::string &msg) {
	if (msg.size() < 2 || msg.substr(msg.size() - 2) != "\r\n")
		client.buffer_out += msg + "\r\n";
	else
		client.buffer_out += msg;
}

void Server::broadcast(Channel &chan, const std::string& msg, int senderFd) {
	const std::map<int, Client*>& members = chan.getMembers();
	std::vector<int> staleMembers;

	for (Channel::ConstMemberIt it = members.begin(); it != members.end(); ++it) {
		if (it->first == senderFd) 
			continue;
		ClientIt clientIt = clients.find(it->first);
		if (clientIt == clients.end()) {
			staleMembers.push_back(it->first);
			continue;
		}
		sendToClient(clientIt->second, msg);
		std::cout << "[Broadcast to " << clientIt->second.getNick() << "] " << msg << std::endl;
	}
	for (size_t i = 0; i < staleMembers.size(); ++i)
		chan.removeMember(staleMembers[i]);
}

void Server::handleCap(Client &client, const Command& cmd) {
	if (cmd.params.empty()) 
		return;
	std::string sub = cmd.params[0];
	for (size_t i = 0; i < sub.size(); ++i)
		sub[i] = std::toupper(static_cast<unsigned char>(sub[i]));

	const std::string nickOrStar = client.getNickOrStar();
	const std::string srv = "localhost";

	if (sub == "LS")
		return sendToClient(client, ":" + srv + " CAP " + nickOrStar + " LS :");
	if (sub == "END")
		return;
	sendNumericReply(client, ERR_UNKNOWNCOMMAND, "", "CAP"); 
}

void Server::handlePing(Client &client, const Command& cmd) {
	if (!cmd.params.empty()) 
	{
		std::string reply = "PONG localhost :" + cmd.params[0];
		sendToClient(client, reply);
		std::cout <<  "[Reply]: " << reply << std::endl;
	}
	else 
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "PING");
	return;
}

void Server::handleQuit(Client &client, const Command& cmd) {
	std::string quitMessage = cmd.params.empty() ? "Client Quit" : cmd.params[0];
	closeConnection(client.getFd(), quitMessage, true);
}