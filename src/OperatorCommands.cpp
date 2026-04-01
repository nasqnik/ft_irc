#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Command.hpp"
#include "../include/Utils.hpp"
#include "../include/Channel.hpp"

#include <sstream>

void Server::handleKick(Client &client, const Command& cmd) {

	if (cmd.params.size() < 2) {
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "KICK");
		return;
	}
	std::string channelName = cmd.params[0];
	if (!isValidChannelName(channelName)) {
		sendNumericReply(client, ERR_BADCHANMASK,"", channelName);
		return;
	}
	Channel *channel = findChannel(channelName);
	if (!channel) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL,"", channelName);
		return;
	}

	int opFd = client.getFd();
	if (!channel->isMember(opFd)) {
		sendNumericReply(client, ERR_NOTONCHANNEL,"", channelName);
		return;
	}
	if (!channel->isOperator(opFd)) {
		sendNumericReply(client, ERR_CHANOPRIVSNEEDED,"", channelName);
		return;
	}

	std::string targetNick  = cmd.params[1];
	Client *target = findClientByNick(targetNick);
	if (!target) {
		sendNumericReply(client, ERR_NOSUCHNICK,"",targetNick);
		return;
	}
	if (!channel->isMember(target->getFd())) {
		sendNumericReply(client, ERR_USERNOTINCHANNEL, "", targetNick + " " + channelName);
		return;
	}

	std::string reason;
	if (cmd.params.size() > 2)
		reason = stripMessagePrefix(cmd.params[2]);
	else
		reason = client.getNick();

	std::string prefix = ":" + client.getPrefix();
	std::string kickMsg = prefix + " KICK " + channel->getName() + " " + targetNick + " :" + reason;

	broadcast(*channel, kickMsg, -1);

	channel->removeMember(target->getFd());

	if (channel->getMembers().empty())
		channels.erase(ircCasefold(channelName));
}

void Server::handleMode(Client &client, const Command& cmd) {

	if (cmd.params.empty()) {
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "MODE");
		return;
	}
	std::string channelName = cmd.params[0];
	if (channelName.empty() || (channelName[0] != '#' && channelName[0] != '&')) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL, "", cmd.params[0]);
		return;
	}
	Channel *chan = findChannel(channelName);
	if (!chan) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL,"", channelName);
		return;
	}
	int opFd = client.getFd();
	if (!chan->isMember(opFd)) {
		sendNumericReply(client, ERR_NOTONCHANNEL,"", channelName);
		return;
	}
	if (!chan->isOperator(opFd)) {
		sendNumericReply(client, ERR_CHANOPRIVSNEEDED,"", channelName);
		return;
	}
	if (cmd.params.size() == 1) 
	{
		std::string modes = chan->channelModeStr();
		std::string modeParams;
		std::string key = chan->getKey();

		if (!key.empty())
			modeParams += " " + chan->getKey();
		if (chan->getUserLimit() != -1)
		{
			std::ostringstream oss;
			oss << chan->getUserLimit();
			std::string limitStr = oss.str();
			modeParams += " " + limitStr;
		}
		sendNumericReply(client, RPL_CHANNELMODEIS, "",
			chan->getName() + " " + (modes.empty() ? "+" : modes) + modeParams);
		return;
	}
	std::string flag = cmd.params[1];
	if (flag.empty() || (flag[0] != '+' && flag[0] != '-')) {
		sendNumericReply(client, ERR_UNKNOWNMODE, "", std::string(1, flag.empty() ? '?' : flag[0]));
		return;
	}
	changeChannelMode(client, *chan, cmd);
}

void Server::changeChannelMode(Client &client, Channel &chan, const Command& cmd)
{
	bool mode = true;
	std::string flag = cmd.params[1];
	size_t index = 2;

	for (size_t i = 0; i < flag.size(); ++i) 
	{
		char c = flag[i];
		if (c == '-') {
			mode = false;
			continue;
		}
		if (c == '+') {
			mode = true;
			continue;
		}

		bool applied = true;
		switch (c) 
		{
			case 'i':
				chan.setInviteOnly(mode);
				break;
			case 't':
				chan.setTopicRestricted(mode);
				break;
			case 'k':
				applied = modeK(client, chan, cmd, mode, index);
				break;
			case 'o':
				applied = modeO(client, chan, cmd, mode, index);
				break;
			case 'l':
				applied = modeL(client, chan, cmd, mode, index);
				break;

			default:
				sendNumericReply(client, ERR_UNKNOWNMODE,"", std::string(1, c));
				applied = false;
				break;
		}
		if (applied) {
			std::string prefix = ":" + client.getPrefix();
			std::string modeFlag = std::string(1, (mode ? '+' : '-')) + std::string(1, c);
			std::string msg = prefix + " MODE " + chan.getName() + " " + modeFlag;

			if ((c == 'k' && mode && index > 2) ||
				(c == 'o' && index > 2) ||
				(c == 'l' && mode && index > 2)) {
				msg += " " + cmd.params[index - 1];
			}
			broadcast(chan, msg, -1);
		}
	}
}

bool Server::modeK(Client &client, Channel &chan, const Command& cmd, bool mode, size_t& index) 
{
	if (mode)
	{
		if (cmd.params.size() <= index || cmd.params[index].empty()) {
			sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "MODE k");
			return false;
		}
		chan.setKey(cmd.params[index]);
		++index;
	}
	else
		chan.clearKey();
	return true;
}

bool Server::modeO(Client &client, Channel &chan, const Command& cmd, bool mode, size_t& index) 
{
	if (index >= cmd.params.size()) {
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "MODE o");
		return false;
	}
	Client *target = findClientByNick(cmd.params[index++]);
	if (!target || !chan.isMember(target->getFd())) {
		sendNumericReply(client, ERR_USERNOTINCHANNEL, "", 
			cmd.params[index - 1] + " " + chan.getName());
		return false;
	}
	if (mode)
		chan.addOperator(target->getFd());
	else 
		chan.removeOperator(target->getFd());
	return true;
}

bool Server::modeL(Client &client, Channel &chan, const Command& cmd, bool mode, size_t& index) 
{
	if (mode) 
	{
		if (index >= cmd.params.size()) {
			sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "MODE l");
			return false;
		}
		const std::string &arg = cmd.params[index++];
		bool valid = true;
		for (size_t i = 0; i < arg.size(); ++i) 
		{
			if (!isdigit(arg[i])) 
			{
				valid = false;
				break;
			}
		}
		if (!valid || arg.empty()) {
			sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "MODE l");
			return false;
		}
		std::istringstream iss(arg);
		int limit;
		iss >> limit;
		if (iss.fail() || limit <= 0) {
			sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "MODE l");
			return false;
		}
		chan.setUserLimit(limit);
	} else
		chan.clearUserLimit();
	return true;
}

void Server::handleTopic(Client &client, const Command& cmd) {

	if (cmd.params.empty() || cmd.params.size() > 2) {
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "TOPIC");
		return;
	}

	std::string channelName = cmd.params[0];
	if (!isValidChannelName(channelName)) {
		sendNumericReply(client, ERR_BADCHANMASK, "", channelName);
		return;
	}
	Channel *channel = findChannel(channelName);
	if (!channel) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL, "", channelName);
		return;
	}

	if (cmd.params.size() == 1) {
		if (channel->getTopic().empty())
			sendNumericReply(client, RPL_NOTOPIC, "", channelName);
		else
			sendNumericReply(client, RPL_TOPIC, channelName + " :" + channel->getTopic());
		return;
	}

	if (channel->isTopicRestricted() && !channel->isOperator(client.getFd())) {
		sendNumericReply(client, ERR_CHANOPRIVSNEEDED,"", channelName);
		return;
	}

	std::string newTopic = stripMessagePrefix(cmd.params[1]);
	channel->setTopic(newTopic);

	std::string topicMsg = ":" + client.getPrefix() + " TOPIC " + channelName + " :" + newTopic;
	broadcast(*channel, topicMsg, -1);

}

void Server::handleInvite(Client &client, const Command& cmd) {

	if (cmd.params.size() != 2) {
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "INVITE");
		return;
	}

	std::string targetNick = cmd.params[0];
	Client *target = findClientByNick(targetNick);
	if (!target) {
		sendNumericReply(client, ERR_NOSUCHNICK,"",targetNick);
		return;
	}

	std::string channelName = cmd.params[1];
	if (!isValidChannelName(channelName)) {
		sendNumericReply(client, ERR_BADCHANMASK,"", channelName);
		return;
	}
	Channel *channel = findChannel(channelName);
	if (!channel) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL,"", channelName);
		return;
	}
	if (!channel->isMember(client.getFd())) {
		sendNumericReply(client, ERR_NOTONCHANNEL,"", channelName);
		return;
	}
	if (channel->isInviteOnly() && !channel->isOperator(client.getFd())) {
		sendNumericReply(client, ERR_CHANOPRIVSNEEDED, "", channelName);
		return;
	}
	if (channel->isMember(target->getFd())) {
		sendNumericReply(client, ERR_USERONCHANNEL, "", targetNick + " " + channelName);
		return;
	}

	channel->addInvite(target->getFd());

	sendNumericReply(client, RPL_INVITING, targetNick, channelName);

	std::string msg = ":" + client.getPrefix() + " INVITE " + targetNick + " :" + channelName;
	sendToClient(*target, msg);
	std::cout << "[Reply] " << msg << std::endl; 
}