NAME		= pulsar

BUILDDIR	= build/
SRCDIR		= src/
INCDIR		= include/

PULSAR_SRC	= $(SRCDIR)main.cpp

PULSAR_OBJS	= $(PULSAR_SRC:.cpp=.o)
OBJS		= $(PULSAR_OBJS)

PULSAR_INC	= -I $(INCDIR)

CXXFLAGS	+= -std=c++17
CXXFLAGS	+= -Wall
CXXFLAGS	+= -Wextra
CXXFLAGS	+= $(PULSAR_INC)

LDFLAGS		+= -lvulkan

CXX			= g++
ECHO		= echo
RM			= rm -f


all: $(NAME)

dir:
	@mkdir $(BUILDDIR) 2> /dev/null || true

$(NAME): dir $(OBJS)
	@$(ECHO) $(CXX) -o $(NAME) $(LDFLAGS)
	@$(CXX) -o $(BUILDDIR)$(NAME) $(OBJS) $(LDFLAGS)

clean:
	@$(ECHO) $(RM) OBJS
	@$(RM) $(OBJS)

fclean: clean
	$(RM) $(BUILDDIR)$(NAME)

re: fclean all

dbg: CXXFLAGS += -g
dbg: re

.cpp.o:
	@echo $(CXX) -c $(CXXFLAGS) $<
	@$(CXX) -c $(CXXFLAGS) $< -o $@
