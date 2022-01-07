//
// tcp_server.cpp
//
// Defines the entry point for the console application.

#include "stdafx.h"
#include "../SocketLib/SocketLib.h"

using namespace std;

int main() {

	try {
		Socket listenSocket(SocketType::Tcp);
		listenSocket.Bind(Endpoint("0.0.0.0", 5959));
		listenSocket.Listen();
		cout << "Server started." << "\n";

		Socket tcpConnection;
		string ignore;
		listenSocket.Accept(tcpConnection, ignore);

		auto a = tcpConnection.GetPeerAddr().ToString();
		cout << "Socket from " << a << " is accepted." << "\n";
		while (true) {
			string receivedData;
			cout << "Receiving data..." << "\n";
			int result = tcpConnection.Receive();

			if (result == 0) {
				cout << "Connection closed." << "\n";
				break;
			}
			else if (result < 0) {
				cout << "Connection lost: " << GetLastErrorAsString() << "\n";
			}

			cout << "Received: " << tcpConnection.m_receiveBuffer << "\n";
		}

		tcpConnection.Close();
	}
	catch (Exception& e) {
		cout << "Exception! " << e.what() << endl;
	}

	return 0;
}