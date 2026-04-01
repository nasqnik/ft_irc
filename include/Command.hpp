#ifndef COMMAND_HPP
# define COMMAND_HPP

#include <string>
#include <iostream>
#include <vector>

class Command {
	public:
		std::string		      name;
		std::vector<std::string> params;
		bool	isColon;

		Command();
		Command(const std::string &n, const std::string &p); // for testing
		Command(const std::string &n, const std::vector<std::string> &p);
		~Command();
};

#endif