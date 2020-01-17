#include "TCPServer.h"
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex>
#include "exceptions.h"

TCPServer::TCPServer() {
}


TCPServer::~TCPServer() {
}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {
	std::cout << "Ip address is: " << ip_addr << std::endl;

	// create socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	// ensure created successfully
	if (sockfd == 0) 
			throw socket_error("Failed to make socket!");
	

	// bind
	sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	sockaddr.sin_port = htons(port);

	int bound = bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));

	// ensure binding succeeds
	if (bound < 0) 
		throw socket_error("Failed to bind to port!");


	this->sockfd = sockfd;
	this->sockaddr = sockaddr;
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
void TCPServer::listenSvr() {
	int listening;

	while (true) {
		if (this->sockfd != -1) 
			listening = listen(this->sockfd, 10);
		else 
			throw socket_error("Failed to listen on socket!");
		

		// ensure listening successful
		if (listening < 0) 
			throw socket_error("Failed to listen on socket!");

		// grab a connection
		auto addrlen = sizeof(this->sockaddr);
		int connection = accept(this->sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);

		if (connection < 0) {
			throw socket_error("Failed to get connection!");
		} else {
			std::cout << "Added new connection: " << connection << std::endl;

			// start client thread
			std::thread clientThread(&TCPServer::clientThread, this, connection);
			clientThread.detach(); // make thread a daemon
			this->clientConns.push_back(connection);
		}
	}
}

/*
	Sends heartbeat every 3 seconds to client
*/
void TCPServer::heartbeatThread(int conn) {
	std::string hbStr = "HEARTBEAT\n";

	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(3));

		try {
			send(conn, hbStr.c_str(), hbStr.length(), 0);
		} catch (std::exception& e) {
			throw std::runtime_error("Failed to send heartbeat to client!");
		}
	}
}

void TCPServer::clientThread(int conn) {
	// start heartbeat thread with client
	std::thread hbThread(&TCPServer::heartbeatThread, this, conn);
	hbThread.detach();

	// send client menu
	auto menuStr = getClientMenu();
	try {
		send(conn, menuStr.c_str(), menuStr.length(), 0);
	} catch (std::exception& e) {
		throw std::runtime_error("Failed to send to client!");
	}

	std::string promptMessage = "\nEnter a command: \n";
	while (true) {
		// send message to prompt user to enter a command
		try {
			send(conn, promptMessage.c_str(), promptMessage.length(), 0);
		} catch (std::exception& e) {
			throw std::runtime_error("Failed to send to client!");
		}

		// read from connection
		char buffer[2048] = "";
		try {
			auto bytesToRead = read(conn, buffer, sizeof(buffer));
		} catch (std::exception& e) {
			throw std::runtime_error("Failed to read from client socket!");
		}

		std::string userString(buffer);
		auto sanitizedInput = sanitizeUserInput(userString); // sanitize user input

		// respond to user request
		if (sanitizedInput.compare("exit") == 0) // if user wants to exit, break
			break;

		auto response = handleUserString(sanitizedInput) + "\n";
		send(conn, response.c_str(), response.length(), 0);
	}

	close(conn);
	std::cout << "Closed connection with client " << conn << std::endl;
}

std::string TCPServer::handleUserString(std::string userString) {
	if (userString.compare("hello") == 0) {
		return "hello client!";
	} else if (userString.compare("1") == 0) {
		return this->serverName;
	} else if (userString.compare("2") == 0) {
		char *s = inet_ntoa(sockaddr.sin_addr);
		return (std::string) s;
	} else if (userString.compare("3") == 0) {
		return this->serverOwner;
	} else if (userString.compare("4") == 0) {
		return "Java and Python";
	} else if (userString.compare("5") == 0) {
		return "Bellbrook, Ohio, USA";
	} else if (userString.compare("passwd") == 0) {
		return "Currently unavailable feature";
	} else if (userString.compare("menu") == 0) {
		return getClientMenu();
	} else {
		return "Warning: unknown command: " + userString;
	}
}

std::string TCPServer::getClientMenu() {
	std::stringstream ss;

	ss << "Available Commands:\n";
	ss << "hello - displays a greeting\n";
	ss << "1 - displays server name\n";
	ss << "2 - displays server address\n";
	ss << "3 - displays server owner\n";
	ss << "4 - displays server owners favorite programming languages\n";
	ss << "5 - displays server owners location\n";
	ss << "passwd - currently unavailable\n";
	ss << "exit - closes connection to server\n";
	ss << "menu - displays this menu of available commands\n";

	return ss.str();
}

std::string TCPServer::sanitizeUserInput(const std::string& s) {
	// remove leading/trailing white spaces from user input
	// influence from https://www.techiedelight.com/trim-string-cpp-remove-leading-trailing-spaces/
	std::string leftTrimmed = std::regex_replace(s, std::regex("^\\s+"), std::string(""));
	std::string leftAndRightTrimmed = std::regex_replace(leftTrimmed, std::regex("\\s+$"), std::string(""));
	
	// convert string to lowercase
	// influence from https://stackoverflow.com/questions/313970/how-to-convert-stdstring-to-lower-case
	std::transform(leftAndRightTrimmed.begin(), leftAndRightTrimmed.end(), leftAndRightTrimmed.begin(),
    [](unsigned char c){ return std::tolower(c); });

	return leftAndRightTrimmed;
}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/
void TCPServer::shutdown() {
	// close all client sockets
	for(int cliConn : this->clientConns)
		close(cliConn);


	// close socket
	close(this->sockfd);
}
