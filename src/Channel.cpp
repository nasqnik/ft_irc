#include "../include/Channel.hpp"
#include "../include/Client.hpp"

Channel::Channel() : name(""), topic(""), inviteOnly(false),
					topicRestricted(false), userLimit(-1) {};

Channel::Channel(const std::string &n) : name(n), topic(""), 
					inviteOnly(false), topicRestricted(false), userLimit(-1) { };

Channel::~Channel() {};


// ==== Identity ====

const std::string& Channel::getName() const {
	return name;
}

void Channel::setName(const std::string& n) {
	name = n;
}

const std::string& Channel::getTopic() const {
	return topic;
}

void Channel::setTopic(const std::string& t) {
	topic = t;
}


// ==== Members ====

const std::map<int, Client*>& Channel::getMembers() const {
	return members;
}

bool Channel::isMember(int fd) const {
	return members.find(fd) != members.end();
}

void Channel::addMember(Client &client, bool makeOperator) {
	int fd = client.getFd();
	members[fd] = &client;
	if (makeOperator)
        operators.insert(fd);
}

void Channel::removeMember(int fd) {
    members.erase(fd);
    operators.erase(fd);
}


// ==== Operators ====

bool Channel::isOperator(int fd) const {
    return operators.find(fd) != operators.end();
}

void Channel::addOperator(int fd) {
    if (isMember(fd))
		operators.insert(fd);
}

void Channel::removeOperator(int fd) {
    operators.erase(fd);
}


// ==== Invites ====

void Channel::addInvite(int fd) {
    invited.insert(fd);
}

bool Channel::isInvited(int fd) const {
    return invited.find(fd) != invited.end();
}

void Channel::removeInvite(int fd) {
    invited.erase(fd);
}


// ==== Modes ====

bool Channel::isInviteOnly() const {
     return inviteOnly; 
}
    
void Channel::setInviteOnly(bool val) {
    inviteOnly = val; 
}

bool Channel::isTopicRestricted() const { 
    return topicRestricted; 
}

void Channel::setTopicRestricted(bool val) { 
    topicRestricted = val; 
}

const std::string& Channel::getKey() const { 
    return key; 
}

void Channel::setKey(const std::string& k) { 
    key = k; 
}

void Channel::clearKey() { 
    key.clear(); 
}

int Channel::getUserLimit() const { 
    return userLimit; 
}

void Channel::setUserLimit(int limit) { 
    userLimit = limit; 
}

void Channel::clearUserLimit() { 
    userLimit = -1; 
}


std::string Channel::channelModeStr() const {

    std::string mode = "+";

    if (isInviteOnly())
		mode += "i";
    if (isTopicRestricted())
		mode += "t";
    if (!key.empty())
		mode += "k";
    if (userLimit > 0)
		mode += "l";
    
    return mode;
}
