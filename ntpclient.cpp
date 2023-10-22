/*
* MIT License
* Copyright (C) 2023 Edison Henrique Andreassy
*/

#include <iomanip>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

int main(int argc, char* argv[]) {

	auto sync = false;
	char server[50] = "pool.ntp.org";
	auto port = 123;

	// Check for command line arguments
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-server") == 0)
			strcpy_s(server, argv[++i]);
		else if (strcmp(argv[i], "-port") == 0)
			port = atoi(argv[++i]);
		else if (strcmp(argv[i], "-sync") == 0)
			sync = true;
	}

	// Initiates use of the Winsock DLL
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != S_OK) {
		cerr << "Winsock startup failed." << endl;
		return EXIT_FAILURE;
	}

	// Translate from an ANSI host name to an address
	addrinfo* info = nullptr;
	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	if (getaddrinfo(server, nullptr, &hints, &info) != S_OK) {
		cerr << "Get address info failed." << endl;
		WSACleanup();
		return EXIT_FAILURE;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = reinterpret_cast<sockaddr_in*>(info->ai_addr)->sin_addr;

	freeaddrinfo(info);

	// Creates a UDP socket
	auto sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET) {
		cerr << "Socket creation failed." << endl;
		WSACleanup();
		return EXIT_FAILURE;
	}

	// Set the socket timeout to 5 seconds
	constexpr auto TIME_OUT = 5000;
	auto timeout = reinterpret_cast<const char*>(&TIME_OUT);
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, timeout, sizeof(TIME_OUT));
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, timeout, sizeof(TIME_OUT));

	constexpr auto PACKET_SIZE = 48;

	// Send the NTP packet with the flags li=0, vn=4, mode=3
	const char request[PACKET_SIZE] = { 0x23 };
	if (sendto(sock, request, PACKET_SIZE, 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
		cerr << "Socket send failed." << endl;
		closesocket(sock);
		WSACleanup();
		return EXIT_FAILURE;
	}

	// Receive the NTP packet
	char response[PACKET_SIZE] = { 0 };
	if (recvfrom(sock, response, PACKET_SIZE, 0, nullptr, nullptr) == SOCKET_ERROR) {
		cerr << "Socket receive failed." << endl;
		closesocket(sock);
		WSACleanup();
		return EXIT_FAILURE;
	}

	closesocket(sock);

	// Convert the transmited time-stamp to little-endian
	auto timestamp = ntohl(*reinterpret_cast<unsigned int*>(&response[40]));

	// The number of seconds we get back from the server is equal to the no. of
	// seconds since Jan 1, 1900. Since Unix time starts from Jan 1, 1970, we
	// subtract 70 years worth of seconds from Jan 1, 1990.
	// (1900)-------(1970)************************(Time Packet Left the Server)
	auto ntptime = static_cast<time_t>(timestamp - 2208988800u);

	// Convert to tm structure
	tm t;
	localtime_s(&t, &ntptime);

	if (sync) {
		// Update the computer time
		SYSTEMTIME st{};
		st.wYear = t.tm_year + 1900;
		st.wMonth = t.tm_mon + 1;
		st.wDay = t.tm_mday;
		st.wHour = t.tm_hour;
		st.wMinute = t.tm_min;
		st.wSecond = t.tm_sec;
		if (!SetLocalTime(&st)) {
			cerr << "Set system time failed." << endl;
			WSACleanup();
			return EXIT_FAILURE;
		}
	}

	cout << "NTP time: " << put_time(&t, "%Y-%m-%d %H:%M:%S") << endl;

	WSACleanup();
	return EXIT_SUCCESS;
}
