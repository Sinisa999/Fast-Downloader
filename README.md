# Fast-Downloader

This project is a client–server file transfer application written in C++ using WinSock2.
It allows a client to download a file from the server using single or multiple threads for improved speed.

This project supports multithreaded file downloading where the number of threads can be adjusted depending on the user’s preference. It works with any file type in a binary-safe way (tested with .txt, .jpg, .png, .jepg). The server is capable of handling multiple clients at the same time by using multithreading, while a built-in speed throttling mechanism makes it possible to simulate slower networks and compare performance. Once the segments are downloaded, the client automatically merges them back into the original file. After the process is complete, the program reports the total transfer time and the average speed in MB/s, allowing the user to easily measure the efficiency of single-threaded versus multithreaded transfers.

Downloading a file using single thread: ![Test 2 1](https://github.com/user-attachments/assets/ea436d7c-aec1-4ff4-9f90-5fb4c996adf7)

Downloading a file using multiple threads: ![Test 2 2](https://github.com/user-attachments/assets/0a08848e-66a3-47ea-ae95-659d37769eda)
