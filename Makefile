NAME		= pulsar
TEST_NAME	= run_unittest

BUILDDIR	= build/
SRCDIR		= src/
INCDIR		= include/
SUBMODSDIR 	= submodules/
TESTDIR		= test/
SHADERSDIR	= shaders/
SPVDIR		= $(BUILDDIR)$(SHADERSDIR)

PULSAR_SRC	= $(SRCDIR)main.cpp
PULSAR_OBJS	= $(PULSAR_SRC:.cpp=.o)

OBJS		= $(PULSAR_OBJS)

TEST_SRC	= $(TESTDIR)headers/ColorUnittest.cpp
TEST_OBJS	= $(TEST_SRC:.cpp=.o)

PULSAR_INC	= -I $(INCDIR) \
				-I $(SUBMODSDIR)

SHADERS		= $(SHADERSDIR)default.vert \
				$(SHADERSDIR)default.frag

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
GLSLANGVAL	= glslangValidator


all: $(NAME) test shaders

dir:
	@mkdir $(BUILDDIR) 2> /dev/null || true
	@mkdir $(SPVDIR) 2> /dev/null || true

$(NAME): dir $(OBJS)
	@$(ECHO) $(CXX) -o $(NAME) $(LDFLAGS)
	@$(CXX) -o $(BUILDDIR)$(NAME) $(OBJS) $(LDFLAGS)

test: LDFLAGS += -lgtest -lgtest_main
test: $(TEST_OBJS)
	@$(ECHO) $(CXX) -o $(TEST_NAME) $(LDFLAGS)
	@$(CXX) -o $(TESTDIR)$(TEST_NAME) $(TEST_OBJS) $(LDFLAGS)

shaders: dir
	@$(ECHO) Compiling shaders ...
	@for shader in $(SHADERS); do \
		res_shader=$(SPVDIR)`basename $$shader`.spv ; \
		$(GLSLANGVAL) -V -t $$shader -o $$res_shader \
		&& $(ECHO) ' =>' $$res_shader ; \
	done

clean:
	@$(ECHO) $(RM) OBJS
	@$(RM) $(OBJS)
	@$(ECHO) $(RM) TEST_OBJS
	@$(RM) $(TEST_OBJS)

fclean: clean
	$(RM) $(BUILDDIR)$(NAME)
	$(RM) $(SPVDIR)*.spv
	$(RM) $(TESTDIR)$(TEST_NAME)

re: fclean all

release: CXXFLAGS := $(filter-out $(DBGFLAGS),$(CXXFLAGS))
release: CXXFLAGS += -DNDEBUG
release: $(NAME)

.cpp.o:
	@$(ECHO) $(CXX) -c $(CXXFLAGS) $<
	@$(CXX) -c $(CXXFLAGS) $< -o $@

.PHONY: all test shaders clean fclean release