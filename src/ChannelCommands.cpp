#include "../include/Client.hpp"
#include "../include/Server.hpp"
#include "../include/Command.hpp"
#include "../include/Utils.hpp"
#include "../include/Channel.hpp"

void Server::handleJoin(Client &client, const Command& cmd)
{
	if (cmd.params.empty() || cmd.params[0].empty())
		return sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "JOIN");

	if (cmd.params[0] == "0")
        return partAllChannels(client);

	std::string channelName = cmd.params[0];
	if (!isValidChannelName(channelName)) 
		return sendNumericReply(client, ERR_BADCHANMASK,"", channelName);

	std::string key = ircCasefold(channelName); 
	Channel &channel = channels[key];

	if (channel.getName().empty()) {
		channel.setName(channelName);
		channel.addMember(client, true);
	}
	else {
		if (channel.isMember(client.getFd()))
			return;
		if (channel.isInviteOnly() && !channel.isInvited(client.getFd()))
			return sendNumericReply(client, ERR_INVITEONLYCHAN,"", channelName);
		if (channel.getUserLimit() > 0 &&
            channel.getMembers().size() >= static_cast<size_t>(channel.getUserLimit()))
            return sendNumericReply(client, ERR_CHANNELISFULL,"", channelName);
        if (!channel.getKey().empty()) {
            if (cmd.params.size() < 2 || cmd.params[1] != channel.getKey()){
                return (sendNumericReply(client, ERR_BADCHANNELKEY,"", channelName));
			}
		}
		channel.addMember(client, false);
		if (channel.isInvited(client.getFd()))
			channel.removeInvite(client.getFd());
	}
	std::string msg = ":" + client.getPrefix() + " JOIN " + channelName;
	broadcast(channel, msg, -1);
	sendChannelTopic(client, channel);
	sendChannelNameList(client, channel);
}

void Server::sendChannelTopic(Client &client, Channel &channel)
{
	if (channel.getTopic().empty()) 
		sendNumericReply(client, RPL_NOTOPIC,"", channel.getName());
	else
		sendNumericReply(client, RPL_TOPIC, channel.getName() + " :" + channel.getTopic());
}

void Server::sendChannelNameList(Client &client, Channel &channel)
{
	std::string namesList;
	std::vector<int> staleMembers;

	for (Channel::ConstMemberIt it = channel.getMembers().begin();
		it != channel.getMembers().end(); ++it) {
		ClientIt clientIt = clients.find(it->first);
		if (clientIt == clients.end()) {
			staleMembers.push_back(it->first);
			continue;
		}
		if (channel.isOperator(it->first))
			namesList += "@" + clientIt->second.getNick() + " ";
		else
			namesList += clientIt->second.getNick() + " ";
		}
	for (size_t i = 0; i < staleMembers.size(); ++i)
		channel.removeMember(staleMembers[i]);
	if (!namesList.empty() && namesList[namesList.size()-1] == ' ')
		namesList.erase(namesList.size()-1);
	sendNumericReply(client, RPL_NAMREPLY, "", "= " + channel.getName() + " :" + namesList);
	sendNumericReply(client, RPL_ENDOFNAMES,"", channel.getName());
}

void Server::handlePart(Client &client, const Command& cmd) 
{
	if (cmd.params.empty()) {
		sendNumericReply(client, ERR_NEEDMOREPARAMS, "", "PART");
		return;
	}
	std::string channelName = cmd.params[0];
	if (!isValidChannelName(channelName)) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL,"", channelName);
		return;
	}
	Channel *channel = findChannel(channelName);
	if (!channel) {
		sendNumericReply(client, ERR_NOSUCHCHANNEL,"", channelName);
		return;
	}
	int fd = client.getFd();
	if (!channel->isMember(fd)) {
		sendNumericReply(client, ERR_NOTONCHANNEL,"", channelName);
		return;
	}
	std::string prefix = ":" + client.getPrefix();
	std::string msg = prefix + " PART " + channelName;
	broadcast(*channel, msg, -1);

	channel->removeMember(fd);

	if (channel->getMembers().empty())
		channels.erase(ircCasefold(channelName));
}

void Server::partAllChannels(Client &client) {

	std::vector<std::string> toPart;
    for (ChannelIt it = channels.begin(); it != channels.end(); ++it)
    {
        if (it->second.isMember(client.getFd()))
            toPart.push_back(it->first);
    }

    for (size_t i = 0; i < toPart.size(); ++i) {
        Channel &ch = channels[toPart[i]];
        const std::string &chanName = ch.getName();

        std::string partMsg = ":" + client.getPrefix() + " PART " + chanName;
        broadcast(ch, partMsg, -1);

        ch.removeMember(client.getFd());
		
        if (ch.getMembers().empty())
            channels.erase(toPart[i]);
    }
}