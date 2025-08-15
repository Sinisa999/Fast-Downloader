#include <iostream>
#include <fstream>
#include <winsock2.h>
#include <thread>
#include <vector>
#include <string>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")


#pragma pack(push, 1)
struct SegmentRequest {
	uint8_t command;
	uint32_t offset;
	uint32_t length;
};
#pragma pack(pop)

void downloadSegment(int id, uint32_t offset, uint32_t length) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Thread " << id << ": WSAStartup failed\n";
		return;
	}

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		std::cerr << "Thread: " << id << ": Socket creation failed.\n";
		WSACleanup();
		return;
	}
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(s, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Thread " << id << ": Connection failed\n";
		closesocket(s);
		return;
	}

	SegmentRequest req = {1, offset, length };
	send(s, (char*)&req, sizeof(req), 0);

	char* buffer = new char[length];
	int totalReceived = 0;
	while (totalReceived < length) {
		int bytes = recv(s, buffer + totalReceived, length - totalReceived, 0);
		if (bytes <= 0) break;
		totalReceived += bytes;
	}

	// Saving to part file
	std::string filename = "part" + std::to_string(id) + ".bin";
	std::ofstream outFile(filename, std::ios::binary);
	outFile.write(buffer, totalReceived);
	outFile.close();

	std::cout << "Thread " << id << ": Downloaded " << totalReceived << " bytes.\n";

	delete[] buffer;
	closesocket(s);
	WSACleanup();

}

void mergeParts(int numParts, const std::string& outputFilename) {
	std::ofstream output(outputFilename, std::ios::binary);
	if (!output) {
		std::cerr << "Failed to create output file.\n";
		return;
	}

	for (int i = 0; i < numParts; ++i) {
		std::string partName = "part" + std::to_string(i) + ".bin";
		std::ifstream input(partName, std::ios::binary);

		if (!input) {
			std::cerr << "Failed to open " << partName << "\n";
			continue;
		}

		output << input.rdbuf();
		input.close();
		std::cout << "Merged " << partName << "\n";
	}

	output.close();
	std::cout << "All parts merged into " << outputFilename << "\n";

}

uint32_t requestFileSize() {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(12345);
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (connect(s, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Failed to connect for size request.\n";
		closesocket(s);
		WSACleanup();
		return 0;
	}

	SegmentRequest req = { 0, 0, 0 }; //command 0 = size request
	send(s, (char*)&req, sizeof(req), 0);

	uint32_t fileSize = 0;
	recv(s, (char*)&fileSize, sizeof(fileSize), 0);

	closesocket(s);
	WSACleanup();
	return fileSize;
}

int main() {

	std::cout << "Fast Downloader\n";

	int numThreads;

	std::cout << "Enter number of threads to use (1 = single_threaded): ";
	std::cin >> numThreads;

	if (numThreads < 1) numThreads = 1;

	//Requesting file size from server
	uint32_t totalFileSize = requestFileSize();
	if (totalFileSize == 0) {
		std::cerr << "Failed to retrieve file size from server.\n";
		return 1;
	}

	std::cout << "Total file size: " << totalFileSize << " bytes\n";

	//Calculating segment sizes
	int baseSegmentSize = totalFileSize / numThreads;
	int remainder = totalFileSize % numThreads;

	std::vector<std::thread> threads;
	uint32_t offset = 0;

	auto start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < numThreads; ++i) {
		uint32_t segSize = baseSegmentSize;
		if (i == numThreads - 1) {
			segSize += remainder;
		}
		threads.emplace_back(downloadSegment, i, offset, segSize);
		offset += segSize;
	}

	//Waiting for all threads to finish 
	for (auto& t : threads) {
		t.join();
	}

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> duration = end - start;

	std::cout << "All threads finished downloading.\n";
	std::cout << "Download completed in " << duration.count() << " seconds.\n";

	mergeParts(numThreads, "cat_copy.jpg");

	double totalMB = totalFileSize / (1024.0 * 1024.0);
	double speedMBps = totalMB / duration.count();
	std::cout << "Average download speed: " << speedMBps << " MB/s\n";

	return 0;


}