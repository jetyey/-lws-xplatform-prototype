
CXX = g++

SRC = ../../dev/jds19

INCLUDES = -I ../../lib/libwebsockets/linux_a64/include
LDFLAGS = -static -L../../lib/libwebsockets/linux_a64/lib -lwebsockets -L../../lib/openssl/linux_a64/lib -lssl -lcrypto -lz
LIBS = -lpthread -ldl

CXXFLAGS = -g -Wall ${INCLUDES}
CXXFLAGS += -D_LINUX_SOURCE

all:           	lws-test-server lws-test-client

lws-test-server:	$(SRC)/my-lws-server.o
	$(CXX) $(CXXFLAGS) $(SRC)/my-lws-server.o -o lws-test-server $(LDFLAGS) $(LIBS)

$(SRC)/my-lws-server.o:	$(SRC)/my-lws-server.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC)/my-lws-server.cpp -o $(SRC)/my-lws-server.o

lws-test-client:	$(SRC)/my-lws-client.o
	$(CXX) $(CXXFLAGS) $(SRC)/my-lws-client.o -o lws-test-client $(LDFLAGS) $(LIBS)

$(SRC)/my-lws-client.o:	$(SRC)/my-lws-client.cpp
	$(CXX) $(CXXFLAGS) -c $(SRC)/my-lws-client.cpp -o $(SRC)/my-lws-client.o

clean:
	$(RM) -f $(SRC)/*.o


