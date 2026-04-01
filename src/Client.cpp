#include "../include/Client.hpp"

Client::Client(): fd(-1), nick(""), user(""), registered(false), hasNick(false), hasUser(false), passOk(false) {};

Client::Client(int socket_fd)
	: fd(socket_fd), nick(""), user(""), registered(false), hasNick(false), hasUser(false), passOk(false) 
{};

Client::~Client() {};

// ==== Client's identity ====

int Client::getFd() const {
	return fd;
}

const std::string& Client::getNick() const {
	return nick;
}

std::string Client::getNickOrStar() const {
    return nick.empty() ? "*" : nick;
}

void Client::setNick(const std::string& n) {
	nick = n;
}

const std::string& Client::getUser() const {
	return user;
}

void Client::setUser(const std::string& u) {
	user = u;
}

std::string Client::getPrefix() const {
    return this->getNick() + "!" + this->getUser() + "@localhost";
}

const std::string& Client::getMode() const {
	return mode;
};

void Client::setMode(const std::string& m) {
	mode = m;
};

const std::string& Client::getUnused() const {
	return unused;
};

void Client::setUnused(const std::string& un) {
	unused = un;
};

const std::string& Client::getRealName() const {
	return realname;
};

void Client::setRealName(const std::string& n) {
	realname = n;
};

// ==== Registration flags ====

bool Client::getHasNick() const {
	return hasNick;
}

void Client::setHasNick(bool status) {
	hasNick = status;
}

bool Client::getHasUser() const {
	return hasUser;
}

void Client::setHasUser(bool status) {
	hasUser = status;
}

bool Client::getPassOk() const {
	return passOk;
};

void Client::setPassOk(bool status) {
	passOk = status;
};

bool Client::getRegistered() const {
	return registered;
}

void Client::setRegistered(bool status) {
	registered = status;
};

