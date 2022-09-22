#pragma once

#include "Connection.h"

class TCPConnectionClient : public Connection {
private:
	int tcpsockfd;
public:
	TCPConnectionClient(const char* timeout_) ;
	int Init(const char* address, int port);
	int Send(std::string line) override;
	int Receive() override;
	int Block(bool block) override;
	int Close() override;
};
TCPConnectionClient::TCPConnectionClient(const char* timeout_) : Connection(timeout_) {
	if (WSANOTINITIALISED != 0) {
		WSADATA ws = { 0 };
		if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
			WSAGetLastError();
	}
}

class UDPConnectionClient : public Connection {
private:
	int udpsocket;
public:
	UDPConnectionClient(const char* timeout_);
	int Init(const char* address, int port);
	int Send(std::string line) override;
	int Receive() override;
	int Block(bool block) override;
	int Close() override;
};

UDPConnectionClient::UDPConnectionClient(const char* timeout_) : Connection(timeout_) {
	if (WSANOTINITIALISED != 0) {
		WSADATA ws = { 0 };
		if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
			WSAGetLastError();
	}
}

int TCPConnectionClient::Init(const char* address, int port) {
	WSADATA ws = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &ws) == 0)
	{
		tcpsockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (tcpsockfd < 0)
			error("ERROR:init TCP client");
		#ifdef _MSC_VER
			ZeroMemory((char*)&serv_addr, sizeof(serv_addr));
		#else
			bzero((char*)&serv_addr, sizeof(serv_addr));
		#endif // _MSC_VER
		#ifdef _MSC_VER
			ZeroMemory((char*)&cli_addr, sizeof(cli_addr));
		#else
			bzero((char*)&cli_addr, sizeof(cli_addr));
		#endif // _MSC_VER
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(address);
		serv_addr.sin_port = htons(port);
		int servlen = sizeof(serv_addr);
		number = connect(tcpsockfd, (struct sockaddr*) & serv_addr, servlen);
		if (number < 0)
			error("ERROR:connect TCP client");
		if (setsockopt(tcpsockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout) < 0)
			error("ERROR:setsockopt failed");
	}
	else
		WSAGetLastError();
	return number;
}

int UDPConnectionClient::Init(const char* address, int port) {
	//WSADATA ws = { 0 };
	//if (WSAStartup(MAKEWORD(2, 2), &ws) == 0)
	if (WSANOTINITIALISED != 0)
	{
		udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (udpsocket < 0)
			error("ERROR:init UDP client");
		memset((char*)&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(address);
		serv_addr.sin_port = htons(port);
	}
	else
		WSAGetLastError();
	return number;
}

int UDPConnectionClient::Send(std::string line) {
	number = sendto(udpsocket, line.c_str(), buflen, 0, (struct sockaddr*) & serv_addr, sizeof serv_addr);
	if (number < 0)
		error("ERROR on send udp");
	return number;
}

int UDPConnectionClient::Receive() {

	return number;
}

int TCPConnectionClient::Receive() {
	memset(buffer, 0, buflen);
	number = recv(tcpsockfd, buffer, buflen, 0);
	//std::cout << buffer << std::endl;
	if (number < 0)
		error("ERROR on read");
	for (int i = 0; i < strlen(buffer); i++)
		strCli += buffer[i];
	if (veccli.size() != 0 && strCli == veccli[veccli.size() - 1])
		return number;//проверяем не повторилась ли передача tcp пакета
	veccli.push_back(strCli);
	strCli = "";
	return number;
}

int TCPConnectionClient::Send(std::string line) {
	number = send(tcpsockfd, line.c_str(), buflen, 0);
	if (number < 0)
		error("ERROR Client TCP send");
	return number;
}

int UDPConnectionClient::Close() {
	shutdown(udpsocket, 0);
	closesocket(udpsocket);
	Connection::Close();
	return 0;
}

int TCPConnectionClient::Close() {
	shutdown(tcpsockfd, 0);
	closesocket(tcpsockfd);
	Connection::Close();
	return 0;
}
int UDPConnectionClient::Block(bool block) {
	u_long argp = block ? 1 : 0;
	return number;
}
int TCPConnectionClient::Block(bool block) {
	u_long argp = block ? 1 : 0;
	return number;
}