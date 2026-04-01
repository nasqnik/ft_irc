#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <iostream>
# include <string>
# include <map>
# include <set>

class Client;

class Channel {

	private:
		std::string name;
		std::string topic;
		bool		inviteOnly;
		bool		topicRestricted;
		std::string key;
		int		 userLimit;

		std::map<int, Client*>  members;
		std::set<int>		   operators;
		std::set<int>		   invited;

	public:
		typedef std::map<int, Client*>::iterator MemberIt;
		typedef std::map<int, Client*>::const_iterator ConstMemberIt;

		Channel();
		Channel(const std::string &n);
		~Channel();

		// ==== Identity ====
		const std::string& getName() const;
		void setName(const std::string& n);

		const std::string& getTopic() const;
		void setTopic(const std::string& t);

		// ==== Members ====
		const std::map<int, Client*>& getMembers() const;

		bool isMember(int fd) const;
		void addMember(Client &client, bool makeOperator = false);
		void removeMember(int fd);

		// ==== Operators ====
		bool isOperator(int fd) const;
		void addOperator(int fd);
		void removeOperator(int fd);

		// ==== Invites ====
		void addInvite(int fd);
		bool isInvited(int fd) const;
		void removeInvite(int fd);

		 // ==== Modes ====
		bool isInviteOnly() const;
		void setInviteOnly(bool val);

		bool isTopicRestricted() const;
		void setTopicRestricted(bool val);

		const std::string& getKey() const;
		void setKey(const std::string& k);
		void clearKey();

		int getUserLimit() const;
		void setUserLimit(int limit);
		void clearUserLimit();

		std::string channelModeStr() const;
};

#endif