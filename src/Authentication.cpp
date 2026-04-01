#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Command.hpp"
#include "../include/Utils.hpp"
#include "../include/Channel.hpp"

#include <sstream>

void Server::handleRegistration(Client &client, const Command &cmd) {
	if (cmd.name == "PASS")
		validatePass(client, cmd.params.size() > 0 ? cmd.params[0] : "");
	else if (cmd.name == "NICK") 
		validateNick(client, cmd.params.size() > 0 ? cmd.params[0] : "");
	else if (cmd.name == "USER")
		validateUser(client, cmd.params);
	else
		sendNumericReply(client, ERR_NOTREGISTERED);
}

void Server::finishRegistration(Client &client)
{
	if (client.getRegistered())
		return;
	if (!client.getHasNick() || !client.getHasUser())
		return;
	if (!client.getPassOk()) {
		sendNumericReply(client, ERR_NOTREGISTERED);
		return;  // Keep the socket alive, wait for PASS
	}

	client.setRegistered(true);
		
	std::string welcomeMsg = replies[RPL_WELCOME] + client.getPrefix();
	sendNumericReply(client, RPL_WELCOME, welcomeMsg);
}
		
void Server::validatePass(Client &client, const std::string &param) {
		
	if (client.getRegistered())
		return sendNumericReply(client, ERR_ALREADYREGISTRED);

	if (param.empty() || param != this->password ) {
		sendNumericReply(client, ERR_PASSWDMISMATCH);
		closeConnection(client.getFd());
		return;
	}
	client.setPassOk(true);
}

void Server::validateNick(Client &client, const std::string &param)
{
	if (!client.getPassOk())
		return sendNumericReply(client, ERR_NOTREGISTERED);
	if (param.empty())
		return sendNumericReply(client, ERR_NONICKNAMEGIVEN);
	if (!isValidNick(param))
		return sendNumericReply(client, ERR_ERRONEUSNICKNAME,"",param);
	if (nickInUse(param))
		return sendNumericReply(client, ERR_NICKNAMEINUSE,"", param);
	// Before registration
	if (!client.getRegistered()) {
		client.setNick(param);
		client.setHasNick(true);
		finishRegistration(client);
		return;
	}
	// After registration, just change nickname
	std::string oldNick = client.getNick();
	client.setNick(param);

	announceNickChange(client, oldNick);
}

void Server::announceNickChange(Client &client, const std::string &oldNick)
{
	std::string prefix = ":" + oldNick + "!" + client.getUser() + "@localhost";
	std::string reply  = prefix + " NICK :" + client.getNick();

	for (ChannelIt it = channels.begin(); it != channels.end(); ++it) 
	{
		Channel &chan = it->second;
		if (chan.isMember(client.getFd())) 
			broadcast(chan, reply, client.getFd());
	}
	sendToClient(client, reply);
	std::cout << "[NICK Change] " << reply << std::endl;
}

void Server::validateUser(Client &client, const std::vector<std::string> &params)
{
	// In IRC: USER <username> <mode> <unused> :<realname>

	if (!client.getPassOk()) {
		sendNumericReply(client, ERR_NOTREGISTERED);
		return;
	}
	if (params.size() < 4) {
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "USER");
		return;
	}
	client.setUser(params[0]);
	client.setMode(params[1]);
	client.setUnused(params[2]);

	std::string real;
    for (size_t i = 3; i < params.size(); ++i) {
        if (!real.empty()) 
			real.push_back(' ');
        real += params[i];
    }

	if (!real.empty() && real[0] == ':')
		real.erase(0, 1);

	if (!real.empty() && real[real.size() - 1] == '\r')
		real.erase(real.size() - 1);

    if (real.empty()){
        return (sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "USER"));
	}
	
	client.setRealName(real);
	client.setHasUser(true);
	finishRegistration(client);
} 

bool Server::nickInUse(const std::string &nick) {
	std::string norm = ircCasefold(nick);

	for (ClientIt it = clients.begin(); it != clients.end(); ++it) {
		if (ircCasefold(it->second.getNick()) == norm)
			return true;
	}
	return false;
}

