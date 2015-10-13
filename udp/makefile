CXX = g++
CXXFLAGS = -Wall -ansi -pedantic -g

ifeq ($(shell uname),SunOS)
  LIBS = -lsocket -lnsl
endif

all: delay-server delay-client

delay-client: delay-client.cpp PracticalSocket.cpp PracticalSocket.h libbatscore.a
	$(CXX) $(CXXFLAGS) -o delay-client delay-client.cpp PracticalSocket.cpp libbatscore.a -I../src -I../src/util $(LIBS) -mtune=native -DPRINT_DEBUG -DSSE_DEBUG -DLOG_DEBUG

delay-server: delay-server.cpp PracticalSocket.cpp PracticalSocket.h libbatscore.a
	$(CXX) $(CXXFLAGS) -o delay-server delay-server.cpp PracticalSocket.cpp libbatscore.a -I../src -I../src/util $(LIBS) -mtune=native -DPRINT_DEBUG -DSSE_DEBUG -DLOG_DEBUG

#TCPEchoServer-Thread: TCPEchoServer-Thread.cpp PracticalSocket.cpp PracticalSocket.h
#	$(CXX) -D_GNU_SOURCE -o TCPEchoServer-Thread TCPEchoServer-Thread.cpp PracticalSocket.cpp $(LIBS) -lpthread

clean:
	$(RM) delay-client delay-server