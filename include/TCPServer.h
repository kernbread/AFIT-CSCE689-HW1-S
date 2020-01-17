#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "Server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib> 
#include <unistd.h> 
#include <string>
#include <vector>
#include <thread>

class TCPServer : public Server 
{
public:
   TCPServer();
   ~TCPServer();

   void clientThread(int conn);
   std::string getClientMenu();
   std::string handleUserString(std::string userString);
   std::string sanitizeUserInput(const std::string& s);
   void bindSvr(const char *ip_addr, unsigned short port);
   void listenSvr();
   void shutdown();

private:
 int sockfd = -1;
 std::vector<int> clientConns;

 sockaddr_in sockaddr;
 std::string serverName = "Brian's server";
 std::string serverOwner = "Brian D. Curran Jr.";



};


#endif
