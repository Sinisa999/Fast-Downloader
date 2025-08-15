#define NOMINMAX 

#include <iostream>
#include <winsock2.h>
#include <thread>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

#pragma pack(push, 1)
struct SegmentRequest {
	uint8_t command;
	uint32_t offset;
	uint32_t length;
};
#pragma pack(pop)

void handleClient(SOCKET clientSocket) {
	//Handling each client in a new thread
	SegmentRequest req;
	int bytesReceived = recv(clientSocket, (char*)&req, sizeof(req), 0);
	if (bytesReceived != sizeof(req)) {
		std::cerr << "Faile to receive segment rquest.\n";
		closesocket(clientSocket);
		return;
	}

	if (req.command == 0) {
		//Size request
		FILE* file = fopen("cat.jpg", "rb");
		if (!file) {
			std::cerr << "Failed to open test.txt for size request.\n";
			closesocket(clientSocket);
			return;
		}

		fseek(file, 0, SEEK_END);
		uint32_t fileSize = ftell(file);
		fclose(file);

		send(clientSocket, (char*)&fileSize, sizeof(fileSize), 0);
		closesocket(clientSocket);
		return;
	}
	else {
		std::cout << "Client requested offset: " << req.offset << ", length: " << req.length << "\n";

		FILE* file = fopen("cat.jpg", "rb");
		if (!file) {
			std::cerr << "Failed to open test.txt\n";
			closesocket(clientSocket);
			return;
		}

		fseek(file, req.offset, SEEK_SET);
		char* buffer = new char[req.length];
		size_t readBytes = fread(buffer, 1, req.length, file);
		fclose(file);


		//Simulating limited speed
		const int chunkSize = 256;
		const int delayPerChunksMs = 3;
		int totalSent = 0;

		while (totalSent < readBytes) {
			int toSend = std::min(chunkSize, (int)(readBytes - totalSent));
			int sent = send(clientSocket, buffer + totalSent, toSend, 0);
			if (sent == SOCKET_ERROR) break;
			totalSent += sent;

			std::this_thread::sleep_for(std::chrono::milliseconds(delayPerChunksMs));
		}

		std::cout << "Sent " << totalSent << " bytes to client.\n";

		delete[] buffer;
		closesocket(clientSocket);
	}

}

int main() {

	WSADATA wsaData;
	SOCKET serverSocket, clientSocket;
	sockaddr_in serverAddr, clientAddr;
	int clientSize = sizeof(clientAddr);

	// WinSock Initialization
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed.\n";
		return 1;
	}

	//Creating a socket
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed.\n";
		WSACleanup();
		return 1;
	}

	// Binding socket
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed.\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	// Listening
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed.\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Server is listening on port 12345...\n";


	while (true) {
		clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "Accept failed.\n";
			continue;
		}

		std::cout << "Client connected!\n";

		//Handling each client in a new thread
		std::thread(handleClient, clientSocket).detach();
	}





	//Cleanup
	closesocket(serverSocket);
	WSACleanup();
	return 0;

}
