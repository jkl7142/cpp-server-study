//
// tcp_multi_server_nonblocking.cpp
// 
// multi-thread socket connect

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <signal.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <memory>
#include <vector>
#include <mutex>

#include "../SocketLib/SocketLib.h"

using namespace std;

// true가 되면 프로그램을 종료
volatile bool stopWorking = false;

void ProcessSignalAction(int sig_number) {
	if (sig_number == SIGINT)
		stopWorking = true;
}

int main() {

	// ctrl + c : 종료
	signal(SIGINT, ProcessSignalAction);

	try {
		// 논블록 소켓으로 수 많은 클라이언트를 받아 처리

		// 수 많은 클라가 tcp 5555로 들어오고,
		// hello world 메시지를 보냄
		// 서버는 받은 데이터를 그대로 돌려주는 에코 서버
		// 서버에서는 총 처리한 바이트 수를 지속적으로 출력

		// TCP 연결 각각의 객체
		struct RemoteClient {
			Socket tcpConnection;	// accept한 TCP 연결
		};
		unordered_map<RemoteClient*, shared_ptr<RemoteClient>> remoteClients;

		// TCP 연결을 받는 소켓
		Socket listenSocket(SocketType::Tcp);
		listenSocket.Bind(Endpoint("0.0.0.0", 5555));
		
		listenSocket.SetNonblocking();
		listenSocket.Listen();

		cout << "서버가 시작되었습니다.\n";
		cout << "ctrl-c키를 누르면 프로그램을 종료합니다.\n";

		// 리슨 소켓과 TCP 연결 소켓 모두에 대해서 I/O 가능(avail) 이벤트가 있을 때까지 기다린다.
		// 그리고 나서 I/O 가능 소켓에 대해서 일을 한다.

		// 여기에 넣은 소켓 핸들에 대해서 select나 poll을 한다.
		// 다만, receive나 accept에 대해서만 처리한다.
		vector<PollFD> readFds;
		// 어느 소켓이 어느 RemoteClient에 대한 것인지를 가리킵니다.
		vector<RemoteClient*> readFdsToRemoteClients;

		while (!stopWorking) {
			readFds.reserve(remoteClients.size() + 1);
			readFds.clear();
			readFdsToRemoteClients.reserve(remoteClients.size() + 1);
			readFdsToRemoteClients.clear();

			for (auto i : remoteClients) {
				PollFD item;
				item.m_pollfd.events = POLLRDNORM;
				item.m_pollfd.fd = i.second->tcpConnection.m_fd;
				item.m_pollfd.revents = 0;
				readFds.push_back(item);
				readFdsToRemoteClients.push_back(i.first);
			}

			// 마지막 항목은 리슨 소켓
			PollFD item2;
			item2.m_pollfd.events = POLLRDNORM;
			item2.m_pollfd.fd = listenSocket.m_fd;
			item2.m_pollfd.revents = 0;
			readFds.push_back(item2);

			// I/O 가능 이벤트가 있을 때까지 기다립니다.
			Poll(readFds.data(), (int)readFds.size(), 100);

			// readFds를 수색해서 필요한 처리를 합니다.
			int num = 0;
			for (auto readFd : readFds) {
				if (readFd.m_pollfd.revents != 0) {
					if (num == readFds.size() - 1) {	// 리슨 소켓이면
						// accept를 처리한다.
						auto remoteClient = make_shared<RemoteClient>();

						// 이미 "클라이언트 연결이 들어왔음" 이벤트가 온 상태이므로 그냥 이것을 호출해도 된다.
						string ignore;
						listenSocket.Accept(remoteClient->tcpConnection, ignore);
						remoteClient->tcpConnection.SetNonblocking();

						// 새 클라이언트를 목록에 추가.
						remoteClients.insert({ remoteClient.get(), remoteClient });

						cout << "Client joined. There are " << remoteClients.size() << " connections.\n";
					}
					else {	// TCP 연결 소켓이면
						// 받은 데이터를 그대로 회신한다.
						RemoteClient* remoteClient = readFdsToRemoteClients[num];

						int ec = remoteClient->tcpConnection.Receive();
						if (ec <= 0) {
							// 에러 혹은 소켓 종료.
							// 해당 소켓은 제거
							remoteClient->tcpConnection.Close();
							remoteClients.erase(remoteClient);

							cout << "Client left. There are " << remoteClients.size() << " conncetions.\n";
						}
						else {
							// 받은 데이터 그대로 송신한다.

							// 원칙대로라면 TCP 스트림 특성상 일부만 송신하고 리턴하는 경우도 고려해야 하나, 생략
							remoteClient->tcpConnection.Send(remoteClient->tcpConnection.m_receiveBuffer, ec);
						}
					}
				}
				num++;
			}
		}

		// 사용자가 ctrl-c를 눌러 루프를 빠져나온 상태라면
		listenSocket.Close();
		remoteClients.clear();

	}
	catch (Exception& e) {
		cout << "Exception! " << e.what() << "\n";
	}

	return 0;
}