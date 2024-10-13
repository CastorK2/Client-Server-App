# Compiler settings - Can change to clang if required
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -g -pthread -std=c++17 

# Include directories
INCLUDES = -I. 

LIBARIES = -lspdlog -lfmt -lsodium

# Source files
SRCSC = Connection.cpp Message.cpp BlptSocket.cpp UserInterface.cpp Client.cpp Timer.cpp Logger.cpp
SRCSS = Connection.cpp Message.cpp BlptSocket.cpp UserInterface.cpp Server.cpp Timer.cpp Logger.cpp
SRCSMS = Connection.cpp Message.cpp BlptSocket.cpp UserInterface.cpp MultiplexServer.cpp Timer.cpp Logger.cpp

# Object files
OBJSC = $(SRCSC:.cpp=.o)
OBJSS = $(SRCSS:.cpp=.o)
OBJSMC = $(SRCSMS:.cpp=.o)

# Executable targets
client = client.out
server = server.out
multiserver = multiserver.out

.PHONY: clean

# Default make target
all: client server multiserver

client: $(client)
	@echo $(client) has been created as a executable file.

server: $(server)
	@echo $(server) has been created as a executable file.

multiserver: $(multiserver)
	@echo $(multiserver) has been created as a executable file.

$(client): $(OBJSC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJSC) -o $(client) $(LIBARIES)

$(server): $(OBJSS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJSS) -o $(server) $(LIBARIES)

$(multiserver): $(OBJSMC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJSMC) -o $(multiserver) $(LIBARIES)

# To obtain object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LIBARIES) -c $< -o $@

# To remove generated files
clean:
	rm -f $(OBJSC) $(OBJSS) $(client) $(server) $(multiserver)


