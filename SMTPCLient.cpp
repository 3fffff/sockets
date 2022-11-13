#pragma once

#include "Connection.h"


class SMTPConnectionClient : public Connection {
private:
	int tcpsockfd;
	std::string CreateMail(std::string from, std::string to);
public:
	SMTPConnectionClient(const char* timeout_);
	int Init(const char* address, int port);
	int Send(std::string line) override;
	int Receive() override;
	int Block(bool block) override;
	std::string SendMail(std::string from, std::string to,std::string subject,std::string message);
	int Close() override;
};

SMTPConnectionClient::SMTPConnectionClient(const char* timeout_) : Connection(timeout_) {
	if (WSANOTINITIALISED != 0) {
		WSADATA ws = { 0 };
		if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
			WSAGetLastError();
	}
}

std::string SMTPConnectionClient::SendMail(std::string from, std::string to, std::string subject, std::string message) {
	// приветствуем сервер 
	Send("EHLO SSL\r\n");

	// ждем подтверждение от сервера 
	Receive();

	// начинаем отправлять конверт состоящий из полей 
	// MAIL FROM: и RCPT TO: После каждого поля ждем 
	// подтверждение 
	Send("AUTH LOGIN\r\n");

	Receive();
	// сообщаем отправителя 
	Send("MAIL FROM: [email]"+from+"[/email] ");

	// ждем подтверждение от сервера 
	Receive();

	// сообщаем получателя 
	Send("RCPT TO: [email]"+to+"[/email] ");

	// ждем подтверждение от сервера 
	Receive();

	// подаем команду, что готовы начать передачу письма 
	Send("DATA\r\n");

	// ждем подтверждение от сервера 
	Receive();

	// передаем заголовок 

	// от кого письмо 
	Send("FROM: [email]"+from+"[/email] ");

	// кому письмо 
	Send("TO: [email]"+to+"[/email] ");

	// тема письма 
	Send("SUBJECT: "+subject+"\r\n");

	// текст письма 
	Send(message);

	// говорим, что закончили 
	Send("\r\n.\r\n");
	Receive();

	// прощаемся с сервером 
	Send("QUIT");
	return "OK";
}


int SMTPConnectionClient::Init(const char* address, int port) {
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

int SMTPConnectionClient::Send(std::string line) {
	number = send(tcpsockfd, line.c_str(), buflen, 0);
	if (number < 0)
		error("ERROR Client TCP send");
	return number;
}

int SMTPConnectionClient::Receive() {

	return number;
}

int SMTPConnectionClient::Close() {
	shutdown(tcpsockfd, 0);
	closesocket(tcpsockfd);
	Connection::Close();
	return 0;
}


int SMTPConnectionClient::Block(bool block) {
	u_long argp = block ? 1 : 0;
	return number;
}
