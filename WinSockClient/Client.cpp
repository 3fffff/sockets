#include <iostream>
#include "ClientConnection.h"
#include <fstream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
	/*if (argc < 5)
	{
		std::cout << "no flags args!" << std::endl;
		std::cout << "Usage: client.exe <ip> <portTCP> <portUDP> <folder>" << std::endl;
		exit(1);
	}*/

	Connection* TCPClient = new TCPConnectionClient("500");
	Connection* UDPClient = new UDPConnectionClient("500");
	std::string IP = "127.0.0.1";
	int udpPort = 7000;
	UDPClient->Init(IP.c_str(), udpPort);
	TCPClient->Init(IP.c_str(),6000);
	
	std::vector<std::string> vecline;
	std::string path = "D:\\test.txt";
	std::ifstream file(path);
	if (file.is_open())
	{
		int ch = 0;
		std::string line;
		while ((ch = file.get()) != EOF)
		{
			line += ch;
			if (line.size() == 242) {
				vecline.push_back(line);
				line = "";
			}
		}
		if (line != "")
			vecline.push_back(line);
	}
	file.close();
	std::string filename = path.substr(path.rfind('\\') + 1);
	TCPClient->Send(std::to_string(udpPort));
	TCPClient->Send(filename);
	TCPClient->Send(std::to_string(vecline.size()));
	int zero_nums = 5;
	for (int i = 0; i < vecline.size(); i++)
	{
		std::string id = "Begin";
        std::string istr = std::to_string(0);
        for (int j = 0; j < zero_nums - istr.size(); j++)
            id += '0';
        id += istr + "End";
		//id += vecline[0]; //обертываем udp пакет служебными данными id формата Begin000001EndData
		UDPClient->Send(id);//отправляем udp пакет на сервер
		//Sleep(1000);
		TCPClient->Receive();//получаем подтверждение от сервера через tcp
		std::cout << "i" << i << std::endl;
		if ((i != 0) && (i != atoi(UDPClient->GetBuffer())))//в случае если пакеты
			i--;//потерялись по дороге то уменьшаем i и по циклу состоится
		id = "";//повторная передача
	}
	UDPClient->Close();
	TCPClient->Close();
	delete UDPClient;
	delete TCPClient;
	return 0;
}