#include "../include/Command.hpp"

Command::Command(): name(""), isColon(false) {}

Command::Command(const std::string &n, const std::vector<std::string> &p)
	: name(n), params(p), isColon(false) {}

Command::Command(const std::string &n, const std::string &p)
{
	name = n; 
	for (size_t i=0; i<name.size(); ++i) 
		name[i]=std::toupper(name[i]);
	params.push_back(p);
}

Command::~Command() {};
