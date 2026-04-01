#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <iostream>
# include <string>

class Client {
	private:
		int 		fd;
		std::string nick;
		std::string user;
		std::string mode;
		std::string unused;
		std::string realname;
		
		bool		registered;
		bool		hasNick;
		bool		hasUser;
		bool		passOk;

		
	public:
		Client();
		Client(int socket_fd);
		~Client();
		
		std::string buffer_in; // partial message from client
		std::string buffer_out; // partial message to send
		
		// ==== Client's identity ====
		int getFd() const;

		const std::string& getNick() const;
		std::string getNickOrStar() const;
		void setNick(const std::string& n);
		
		const std::string& getUser() const;
    	void setUser(const std::string& u);

		const std::string& getMode() const;
    	void setMode(const std::string& m);

		const std::string& getUnused() const;
    	void setUnused(const std::string& un);

		const std::string& getRealName() const;
    	void setRealName(const std::string& n);
		
		std::string getPrefix() const;

		// ==== Registration flags ====
		bool getHasNick() const;
		void setHasNick(bool status);

		bool getHasUser() const;
		void setHasUser(bool status);
		
		bool getPassOk() const;
		void setPassOk(bool status);
		
		bool getRegistered() const;
		void setRegistered(bool status);
};

#endif