#define _CRT_SECURE_NO_WARNINGS
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdbool.h>
#include <algorithm>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <cstring>
#include <sstream>
#include <vector>
#include <signal.h>
//#include <format>
#define IPv4_len 15

//link to librarys
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

static void usage(char* cmd) {
	std::cerr << "Usage: %s -t [IP/slash] or [IP]\n";
	exit(1);
}//end usage

static void controlc(int signal) {
	printf("Enter Ctrl-C to Abort. Please waiting...\n");
	WSACleanup();
	exit(signal);
}//end controlc

static u_long lookupAddress(const char* pcHost) {
	struct in_addr address;
	//struct hostent* pHE = NULL;
	//struct addrinfo* result = NULL;
	//struct addrinfo hints;
	int nRemoteAddr;
	bool was_a_name = false;

	inet_pton(AF_INET,pcHost,&(address));//inet_addr(pcHost);

	/*if (nRemoteAddr == INADDR_NONE) {
		// pcHost isn't a dotted IP, so resolve it through DNS
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		int pHE = getaddrinfo(pcHost,0,&hints,&result);//gethostbyname(pcHost);

		if (pHE == 0) {
			return INADDR_NONE;
		}//end if
		nRemoteAddr = *((u_long*)pHE->h_addr_list[0]);
		was_a_name = true;
	}
	if (was_a_name) {
		char address[15];
		std::cout << "DNS: " << pcHost << " is " << inet_ntop(AF_INET, &nRemoteAddr,address,15);//inet_ntoa(address);
	}//end if
	*/
	return address.s_addr;
}//end lookupAddress

static uint32_t WINAPI sendAnARP(IPAddr lpArg) {
	u_long DestIp = lpArg;

	IPAddr SrcIp = 0;       /* default for src ip */
	ULONG MacAddr[2];       /* for 6-byte hardware addresses */
	ULONG PhysAddrLen = 6;  /* default to length of six bytes */
	uint8_t* bPhysAddr;
	uint32_t dwRetVal;

	memset(&MacAddr, 0xff, sizeof(MacAddr));
	PhysAddrLen = 6;

	//SetThreadAffinityMask(GetCurrentThread(), 1);
	//sned arp and wait for reply
	const auto start_time = std::chrono::steady_clock::now();
	dwRetVal = SendARP(DestIp, SrcIp, &MacAddr, &PhysAddrLen);
	const auto end_time = std::chrono::steady_clock::now();

	const auto response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

	//output
	bPhysAddr = (uint8_t*)&MacAddr;

	if (PhysAddrLen) {
		//std::stringstream smacAddr;
		std::string macAddr;
		for (int i = 0; i < (int)PhysAddrLen; i++) {
			if (i == (PhysAddrLen - 1))
				macAddr += std::to_string(bPhysAddr[i]);
			else
				macAddr += std::to_string(bPhysAddr[i]) + ":";
		}//end for
		struct in_addr ip_addr;
		ip_addr.s_addr = DestIp;
		char addr[IPv4_len];
		std::cout << "Reply that " << macAddr << ' ' << inet_ntop(AF_INET, &DestIp,addr, IPv4_len)<< ' ' << response_time << "ms\n";//inet_ntoa
	}//end if
	return 0;
}//end sendAnARP

static u_long getFirstIP(u_long Ip, u_char slash) {
	IPAddr newDestIp = 0;
	int i;
	IPAddr oldDestIp = ntohl(Ip);
	u_long mask = 1 << (32 - slash);
	for (i = 0; i < slash; i++) {
		newDestIp += oldDestIp & mask;
		mask <<= 1;
	}//end for
	//skip network IP
	return ntohl(++newDestIp);
}//end getFirstIP

#define WSA_VERSION MAKEWORD(2, 2) // using winsock 2.2
static bool init_winsock() {
	WSADATA	WSAData = { 0 };
	if (WSAStartup(WSA_VERSION, &WSAData) != 0) {
		// Tell the user that we could not find a usable WinSock DLL.
		if (LOBYTE(WSAData.wVersion) != LOBYTE(WSA_VERSION) ||
			HIBYTE(WSAData.wVersion) != HIBYTE(WSA_VERSION)) {
			std::cerr<<"WSAStartup(): Incorrect winsock version\n";
		}//end if
		WSACleanup();
		return false;
	}//end if
	return true;
}//end init_winsock

static char* getCmdOption(char** begin, char** end, const std::string& option) {
	char** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
		return *itr;

	return 0;
}

int main(int argc, char* argv[]) {
	char* destIpStr = NULL;
	u_char slash = 32;
	int c;
	IPAddr destIp = 0;
	u_long loopcount;
	std::thread* threads;
	u_long i = 0;

	destIpStr = getCmdOption(argv, argv + argc, "-t");

	if (!destIpStr) {
		usage(argv[0]);
	}//end if usage

	if (!init_winsock()) {
		exit(1);
	}//end if

	//get arugment
	strtok(destIpStr, "//");
	char* slash_str = strtok(NULL, "");
	if (slash_str) {
		slash = atoi(slash_str);
	}//end if

	//translate to ulong
	destIp = lookupAddress(destIpStr);

	//move to first IP
	if (slash != 32) {
		destIp = getFirstIP(destIp, slash);
	}//end if

	if (destIp == INADDR_NONE) {
		std::cerr<< "Error: could not determine IP address for "<< destIpStr<< " \n";
		exit(1);
	}//end if

	//set control-c handler
	signal(SIGINT, controlc);

	loopcount = 1 << (32 - slash);
	threads = new std::thread[loopcount];

	//inject
	for (i = 0; i < loopcount; i++) {
		struct in_addr ip_addr;
		ip_addr.s_addr = destIp;

		try {
			threads[i]=std::thread(sendAnARP, destIp);
		}
		catch (std::system_error) {
			std::cerr<< "Could not create Thread.\n";
			exit(1);
		}
		threads[i].detach();

		std::this_thread::sleep_for(std::chrono::microseconds(10));
		u_long temp = ntohl(destIp);
		destIp = ntohl(++temp);
	}//end for

	WSACleanup();

	exit(0);
}//end main
