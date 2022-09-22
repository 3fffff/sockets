#pragma once
#include "Connection.h"

class UDPConnectionServer : public Connection {
private:
	int udpsocket;
public:
	UDPConnectionServer(const char* timeout_);
	int Init(const char* address, int port);
	int Send(std::string line) override;
	int Receive() override;
	int Block(bool block) override;
	int Close() override;
};

UDPConnectionServer::UDPConnectionServer(const char* timeout_) :Connection(timeout_) {
	if (WSANOTINITIALISED == 0) {
		WSADATA ws = { 0 };
		if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
			WSAGetLastError();
	}
}

class TCPConnectionServer : public Connection {
private:
	int tcpsockfd, tcpnewsockfd;
public:
	TCPConnectionServer(const char* timeout_);
	int Init(const char* address, int port);
	int Send(std::string line) override;
	int Receive() override;
	int Block(bool block) override;
	int Close() override;
};
TCPConnectionServer::TCPConnectionServer(const char* timeout_) :Connection(timeout_) {
	if (WSANOTINITIALISED != 0) {
		WSADATA ws = { 0 };
		if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
			WSAGetLastError();
	}
}

int UDPConnectionServer::Init(const char* address, int port) {
	//WSADATA ws = { 0 };
	//if (WSAStartup(MAKEWORD(2, 2), &ws) == 0)
	if (WSANOTINITIALISED !=0)
	{
		udpsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (udpsocket < 0)
			error("ERROR opening socket");
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		std::string str = address;
		std::vector<std::string> octets;
		split(str, '.', octets);
		if (octets.size() == 4)
		{
			serv_addr.sin_addr.S_un.S_un_b.s_b1 = atoi(octets[0].c_str());
			serv_addr.sin_addr.S_un.S_un_b.s_b2 = atoi(octets[1].c_str());
			serv_addr.sin_addr.S_un.S_un_b.s_b3 = atoi(octets[2].c_str());
			serv_addr.sin_addr.S_un.S_un_b.s_b4 = atoi(octets[3].c_str());
		}
		else
			error("ERROR:parse address string");

		if (bind(udpsocket, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) < 0)
			error("ERROR on binding");
		std::cout << "Udp Ready" << std::endl;
	}
	else
		WSAGetLastError();
	return 0;
}
int UDPConnectionServer::Send(std::string line) {
	return number;
}

int UDPConnectionServer::Receive() {
	memset(buffer, 0, buflen);
	int slen = sizeof(sockaddr_in);
	number = recvfrom(udpsocket, buffer, buflen, 0, (struct sockaddr*) & cli_addr, &slen);
	std::cout << "n=" << number << std::endl;
	if (number < 0)
		error("ERROR on read udp");
	for (int i = 0; i < strlen(buffer); i++)
		strFile += buffer[i];
	if (vecline.size() != 0 && strFile == vecline[vecline.size() - 1])
		return number;//проверяем не повторилась ли передача udp пакета
	vecline.push_back(strFile);
	strFile = "";
	return number;
}

int UDPConnectionServer::Block(bool block) {
	u_long argp = block ? 1 : 0;
	return number;
}

int UDPConnectionServer::Close() {
	shutdown(udpsocket, 0);
	closesocket(udpsocket);
	Connection::Close();
	return 0;
}

int TCPConnectionServer::Init(const char* address, int port) {
	//WSADATA ws = { 0 };
	//makeword (y<<2)|8
	if(WSANOTINITIALISED  != 0)
	{
		tcpsockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tcpsockfd < 0)
			error("ERROR opening socket");
#ifdef _MSC_VER
		ZeroMemory((char*)&cli_addr, sizeof(cli_addr));
#else
		bzero((char*)&cli_addr, sizeof(cli_addr));
#endif // _MSC_VER
		memset(serv_addr.sin_zero, 0, sizeof(serv_addr.sin_zero));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(port);
		std::string str = address;
		std::vector<std::string> octets;
		split(str, '.', octets);
		if (octets.size() == 4)
		{
			serv_addr.sin_addr.S_un.S_un_b.s_b1 = atoi(octets[0].c_str());
			serv_addr.sin_addr.S_un.S_un_b.s_b2 = atoi(octets[1].c_str());
			serv_addr.sin_addr.S_un.S_un_b.s_b3 = atoi(octets[2].c_str());
			serv_addr.sin_addr.S_un.S_un_b.s_b4 = atoi(octets[3].c_str());
		}
		else
			error("ERROR:parse address string");

		if (bind(tcpsockfd, (struct sockaddr*) & serv_addr, sizeof(serv_addr)) < 0)
			error("ERROR on binding");
		if (listen(tcpsockfd, SOMAXCONN) < 0)
			error("ERROR on listen");
		std::cout << "Waiting client" << std::endl;
		int clilen = sizeof(cli_addr);
		tcpnewsockfd = accept(tcpsockfd, (struct sockaddr*) & cli_addr, &clilen);
		std::cout << "Client connected" << std::endl;
		if (tcpnewsockfd < 0)
			error("ERROR on accept");

		if (setsockopt(tcpnewsockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof timeout) < 0)
			error("setsockopt failed\n");
	}
	else
		WSAGetLastError();
	return 0;
}

int TCPConnectionServer::Receive() {
	memset(buffer, 0, buflen);
	number = recv(tcpnewsockfd, buffer, buflen, 0);
	if (number < 0)
		error("ERROR on read");
	return number;
}
int TCPConnectionServer::Send(std::string line) {
	return number;
}

int TCPConnectionServer::Block(bool block) {
	u_long argp = block ? 1 : 0;
	ioctlsocket(tcpsockfd, FIONBIO, &argp);
	return number;
}
int TCPConnectionServer::Close() {
	shutdown(tcpnewsockfd, 0);
	shutdown(tcpsockfd, 0);
	closesocket(tcpnewsockfd);
	closesocket(tcpsockfd);
	Connection::Close();
	return 0;
}