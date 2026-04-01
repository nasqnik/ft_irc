#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Command.hpp"
#include "../include/Utils.hpp"
#include "../include/Channel.hpp"

void Server::handlePrivMsg(Client &client, const Command& cmd)
{
	if (cmd.params.empty()) {
		sendNumericReply(client, ERR_NORECIPIENT, "", "PRIVMSG");
		return;
	}
	if (cmd.params.size() == 1) {
		if (cmd.isColon)
			sendNumericReply(client, ERR_NORECIPIENT,"", "PRIVMSG");
		else
			sendNumericReply(client, ERR_NOTEXTTOSEND, "",  "PRIVMSG");
		return;
	}
	std::string msg;
	if (cmd.isColon) 
		msg = cmd.params.back();
	else
		msg = cmd.params[1];

	if (msg.empty()) {
		sendNumericReply(client, ERR_NOTEXTTOSEND, "", "PRIVMSG");
		return;
	}

	std::string target = cmd.params[0];
	if (target[0] == '&' || target[0] == '#')
		messageToChannel(client, target, msg);
	else
		messageToUser(client, target, msg);

}

void Server::messageToChannel(Client &client, const std::string &target, const std::string &msg) {

	Channel *chan = findChannel(target);
	if (!chan) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL, "", target);
		return;
	}
	if (!chan->isMember(client.getFd())) {
		sendNumericReply(client, ERR_CANNOTSENDTOCHAN,"", target);
		return;
	}
	std::string prefix = ":" + client.getPrefix();
	std::string fullMsg = prefix + " PRIVMSG " + chan->getName() + " :" + msg;

	broadcast(*chan, fullMsg, client.getFd());
}

void Server::messageToUser(Client &client, const std::string &targetNick, const std::string &msg) {
	
	Client* target = findClientByNick(targetNick);
	
	if (!target) {
		sendNumericReply(client, ERR_NOSUCHNICK,"",targetNick);
		return;
	}

	std::string prefix = ":" + client.getPrefix();
	std::string fullMsg = prefix + " PRIVMSG " + targetNick + " :" + msg;

	sendToClient(*target, fullMsg);
	std::cout << "[Reply] " << fullMsg << std::endl;
}

void Server::handleNotice(Client &client, const Command& cmd)
{
	if (cmd.params.empty()) 
		return;
    if (cmd.params.size() < 2) 
		return;

	std::string msg;
		if (cmd.isColon) 
			msg = cmd.params.back();
		else
			msg = cmd.params[1];
	
    if (msg.empty()) 
		return;

    const std::string& target = cmd.params[0];

	if (target[0] == '#' || target[0] == '&')
        noticeToChannel(client, target, msg);
    else
        noticeToUser(client, target, msg);
}

void Server::noticeToChannel(Client &client, const std::string &target, const std::string &msg) {
    Channel *chan = findChannel(target);
    if (!chan) 
		return;

    if (!chan->isMember(client.getFd())) 
		return;

    std::string prefix  = ":" + client.getPrefix();
    std::string fullMsg = prefix + " NOTICE " + chan->getName() + " :" + msg;

    broadcast(*chan, fullMsg, client.getFd());
}

void Server::noticeToUser(Client &client, const std::string &targetNick, const std::string &msg) {
    Client* target = findClientByNick(targetNick);
    if (!target) 
		return;

    std::string prefix  = ":" + client.getPrefix();
    std::string fullMsg = prefix + " NOTICE " + targetNick + " :" + msg;

    sendToClient(*target, fullMsg);
}