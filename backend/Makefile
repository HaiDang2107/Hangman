CXX = g++
CXXFLAGS = -std=c++17 -I./include -pthread -Wall -Wextra
LDFLAGS =

# Source files
SOURCES = \
	src/main.cpp \
	src/network/Socket.cpp \
	src/network/EventLoop.cpp \
	src/network/Connection.cpp \
	src/network/Server.cpp \
	src/threading/TaskQueue.cpp \
	src/threading/CallbackQueue.cpp \
	src/threading/Task.cpp \
	src/protocol/packets.cpp

OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = server

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

.PHONY: all clean
