//
// tcp_client.cpp
// 
// Defines the entry point for the console application.

#include "stdafx.h"
#include "../SocketLib/SocketLib.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

int main() {

	try {
		//SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		Socket s(SocketType::Tcp);
		s.Bind(Endpoint::Any);
		cout << "Connecting to server..." << "\n";
		s.Connect(Endpoint("127.0.0.1", 5959));

		cout << "Sending data..." << "\n";
		const char* text = "hello";
		s.Send(text, strlen(text) + 1);

		std::this_thread::sleep_for(1s);

		cout << "Closing socket..." << "\n";
		s.Close();
	}
	catch (Exception& e) {
		cout << "Exception! " << e.what() << endl;
	}

	return 0;
}