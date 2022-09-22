#pragma once
#include <iostream>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <WinSock.h>
#include <fcntl.h>
#pragma comment(lib,"WS2_32.lib")
#include <vector>
#include <sstream>

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) 
		elems.push_back(item);
}

class Connection {
public:
	Connection(const char* timeout);
	virtual int Close();
	virtual int Send(std::string line) = 0;
	virtual int Receive() = 0;
	virtual char* GetBuffer();
	std::vector<std::string> GetVec();
	virtual int Block(bool block) = 0;
	virtual int Init(const char* address, int port) = 0;
	~Connection();
protected:
	struct sockaddr_in serv_addr, cli_addr;
	void bzero(char* buf,int len);
	void error(const char* msg);
	int buflen,number;
	char* buffer;
	std::string strFile, strCli;
	std::vector<std::string> vecline, veccli;
	struct timeval timeout;
};
void Connection::bzero(char* buf, int len) {
	memset(buf, 0, len);
}
void Connection::error(const char* msg) {
	int err = WSAGetLastError();
	perror(msg);
	std::cout << err << std::endl;
	WSACleanup();
	std::cin.ignore();
	exit(1);
}
Connection::Connection(const char* timeout_) {
	buflen = 256;
	buffer = new char[buflen];
	strFile = "";
	strCli = "";
	timeout.tv_sec = 0;
	if (timeout_ != "")
		timeout.tv_usec = atoi(timeout_);
	else
		timeout.tv_usec = 500;
}

char* Connection::GetBuffer() {
	return buffer;
}

std::vector <std::string> Connection::GetVec() {
	return vecline;
}

int Connection::Close() {
	WSACleanup();
	return 0;
}
Connection::~Connection() {
	delete[] buffer;
}