#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "Client.h"
#include <stdio.h> 
#include <arpa/inet.h> 
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <cstdlib> 
#include <unistd.h> 
#include <string>
#include <queue>
#include <mutex>

// The amount to read in before we send a packet
const unsigned int stdin_bufsize = 50;
const unsigned int socket_bufsize = 100;

class TCPClient : public Client
{
public:
   TCPClient();
   ~TCPClient();
	bool sendData(std::string data);
	std::string receiveData();
	void receivingThread();
	void sendingThread();
	std::string sanitizeUserInput(const std::string& s);

   virtual void connectTo(const char *ip_addr, unsigned short port);
   virtual void handleConnection();

   virtual void closeConn();

private:
	 sockaddr_in sockaddr;
	 int sockfd;
	 struct sockaddr_in server;
	 bool connClosed = false;

	 std::queue<std::string> receivedMessages;
	 std::mutex mtx1;
};


#endif
