NAME	= ircserv

FLAGS	= -Wall -Wextra -Werror -std=c++98

CXX 	= c++

SRCS	=	src/main.cpp \
			src/Client.cpp \
			src/Authentication.cpp \
			src/ServerCommands.cpp \
			src/Server.cpp \
			src/Command.cpp \
			src/Channel.cpp \
			src/ChannelCommands.cpp \
			src/MessagingCommands.cpp \
			src/OperatorCommands.cpp \
			src/Utils.cpp 

OBJS	= $(SRCS:.cpp=.o)

RM 		= rm -f

INCL	=	include/Client.hpp \
			include/Channel.hpp \
			include/Server.hpp \
			include/Command.hpp \
			include/NumericReplies.hpp \
			include/Utils.hpp

%.o: %.cpp 
	$(CXX) $(FLAGS) -c $< -o $@

$(NAME): $(OBJS) $(INCL)
	$(CXX) $(FLAGS) $(OBJS) -o $(NAME)

all: $(NAME)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
