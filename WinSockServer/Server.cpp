#include <iostream>
#include "ServerConnection.h"
#include <fstream>
#include <string>

#define FILENAME_MAX 256

int main(int argc, char* argv[]) {
	//system("Del \"D:\\temp\\test.txt\"");
	/*if (argc < 4)
	{
		std::cout << "no flags args!" << std::endl;
		std::cout << "Usage: server.exe <ip> <port> <folder>" << std::endl;
		exit(1);
	}*/

	Connection* TCPServer = new TCPConnectionServer("500");
	TCPServer->Init("127.0.0.1", 6000);
	TCPServer->Receive();
	int udp_port = atoi("7000");
	Connection* UDPServer = new UDPConnectionServer("500");
	UDPServer->Init("127.0.0.1", udp_port);
	TCPServer->Receive();
	std::string file_name = TCPServer->GetBuffer();
	if (file_name.size() > FILENAME_MAX)
		std::cout << "File name is too long" << std::endl;
	TCPServer->Receive();
	int line_count = atoi(TCPServer->GetBuffer());
	std::cout << "start file transmission" << std::endl;
	std::vector<std::string> line_strings;
	int jj = 0;
	for (int i = 0; i < line_count; i++) {
		UDPServer->Receive();
		std::string str = UDPServer->GetBuffer();
		std::string id = UDPServer->GetVec()[i];
		line_strings.push_back(str);
		TCPServer->Send("gfdfgdf");
	}
	std::cout << "Transmission is end" << std::endl;
	std::ofstream outFile;
	outFile.open("D:\\temp\\test.txt");
	for (int i = 0; i < line_count; i++)
		outFile << line_strings[i];
	TCPServer->Close();
	UDPServer->Close();
	delete TCPServer;
	delete UDPServer;

	return 0;
}