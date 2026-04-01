#ifndef UTILS_HPP
# define UTILS_HPP

#include <string>

std::string ircCasefold(const std::string &s);
bool isValidNick(const std::string &nick);
bool isValidChannelName(const std::string &name);
std::string stripMessagePrefix(const std::string &param);

#endif