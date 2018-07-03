NAME		= pulsar
TEST_NAME	= run_unittest

BUILDDIR	= build/
SRCDIR		= src/
INCDIR		= include/
TESTDIR		= test/

PULSAR_SRC	= $(SRCDIR)main.cpp
PULSAR_OBJS	= $(PULSAR_SRC:.cpp=.o)

OBJS		= $(PULSAR_OBJS)

TEST_SRC	= $(TESTDIR)headers/ColorUnittest.cpp
TEST_OBJS	= $(TEST_SRC:.cpp=.o)

PULSAR_INC	= -I $(INCDIR)

DBGFLAGS	+= -g -ggdb

CXXFLAGS	+= -std=c++17
CXXFLAGS	+= -Wall
CXXFLAGS	+= -Wextra
CXXFLAGS	+= $(DBGFLAGS)
CXXFLAGS	+= $(PULSAR_INC)

LDFLAGS		+= -lvulkan
LDFLAGS		+= -lglfw3

CXX			= g++
ECHO		= echo
RM			= rm -f


all: $(NAME) test

dir:
	@mkdir $(BUILDDIR) 2> /dev/null || true

$(NAME): dir $(OBJS)
	@$(ECHO) $(CXX) -o $(NAME) $(LDFLAGS)
	@$(CXX) -o $(BUILDDIR)$(NAME) $(OBJS) $(LDFLAGS)

test: LDFLAGS += -lgtest -lgtest_main
test: $(TEST_OBJS)
	@$(ECHO) $(CXX) -o $(TEST_NAME) $(LDFLAGS)
	@$(CXX) -o $(TESTDIR)$(TEST_NAME) $(TEST_OBJS) $(LDFLAGS)

clean:
	@$(ECHO) $(RM) OBJS
	@$(RM) $(OBJS)
	@$(ECHO) $(RM) TEST_OBJS
	@$(RM) $(TEST_OBJS)

fclean: clean
	$(RM) $(BUILDDIR)$(NAME)
	$(RM) $(TESTDIR)$(TEST_NAME)

re: fclean all

release: CXXFLAGS -= $(DBGFLAGS)
release: CXXFLAGS += -DNDEBUG
release: $(NAME)

.cpp.o:
	@echo $(CXX) -c $(CXXFLAGS) $<
	@$(CXX) -c $(CXXFLAGS) $< -o $@
