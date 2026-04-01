#include "../include/Utils.hpp"

std::string ircCasefold(const std::string &s) {
	std::string result = s;
	for (size_t i = 0; i < result.size(); ++i) 
	{
		char c = result[i];
		if (c >= 'A' && c <= 'Z')
			result[i] = c - 'A' + 'a';
		else if (c == '{') result[i] = '[';
		else if (c == '}') result[i] = ']';
		else if (c == '|') result[i] = '\\';
		else if (c == '^') result[i] = '~';
	}
	return result;
}

bool isValidNick(const std::string &nick)
{
	if (nick.empty() || nick.size() > 9)
		return false;

	const std::string specials = "[]\\`_^{|}-";

	char first = nick[0];
	if (!isalpha(first) && specials.find(first) == std::string::npos)
		return false;

	for (size_t i = 1; i < nick.size(); ++i) {
		char c = nick[i];
		if (!isalnum(c) && specials.find(c) == std::string::npos)
			return false;
	}

	return true;
}


bool isValidChannelName(const std::string &name) {
	if (name.empty() || name.size() > 50)
		return false;
	if (name[0] != '#' && name[0] != '&')
		return false;
	for (size_t i = 1; i < name.size(); ++i) {
		char c = name[i];
		if (c == ' ' || c == ',' || c == 7 || c == ':')
			return false;
	}
	return true;
}

std::string stripMessagePrefix(const std::string &param) {
	if (!param.empty() && param[0] == ':')
		return param.substr(1);
	return param;
}
