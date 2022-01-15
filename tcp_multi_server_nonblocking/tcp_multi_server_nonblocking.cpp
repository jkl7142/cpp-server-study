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

// true�� �Ǹ� ���α׷��� ����
volatile bool stopWorking = false;

void ProcessSignalAction(int sig_number) {
	if (sig_number == SIGINT)
		stopWorking = true;
}

int main() {

	// ctrl + c : ����
	signal(SIGINT, ProcessSignalAction);

	try {
		// ���� �������� �� ���� Ŭ���̾�Ʈ�� �޾� ó��

		// �� ���� Ŭ�� tcp 5555�� ������,
		// hello world �޽����� ����
		// ������ ���� �����͸� �״�� �����ִ� ���� ����
		// ���������� �� ó���� ����Ʈ ���� ���������� ���

		// TCP ���� ������ ��ü
		struct RemoteClient {
			Socket tcpConnection;	// accept�� TCP ����
		};
		unordered_map<RemoteClient*, shared_ptr<RemoteClient>> remoteClients;

		// TCP ������ �޴� ����
		Socket listenSocket(SocketType::Tcp);
		listenSocket.Bind(Endpoint("0.0.0.0", 5555));
		
		listenSocket.SetNonblocking();
		listenSocket.Listen();

		cout << "������ ���۵Ǿ����ϴ�.\n";
		cout << "ctrl-cŰ�� ������ ���α׷��� �����մϴ�.\n";

		// ���� ���ϰ� TCP ���� ���� ��ο� ���ؼ� I/O ����(avail) �̺�Ʈ�� ���� ������ ��ٸ���.
		// �׸��� ���� I/O ���� ���Ͽ� ���ؼ� ���� �Ѵ�.

		// ���⿡ ���� ���� �ڵ鿡 ���ؼ� select�� poll�� �Ѵ�.
		// �ٸ�, receive�� accept�� ���ؼ��� ó���Ѵ�.
		vector<PollFD> readFds;
		// ��� ������ ��� RemoteClient�� ���� �������� ����ŵ�ϴ�.
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

			// ������ �׸��� ���� ����
			PollFD item2;
			item2.m_pollfd.events = POLLRDNORM;
			item2.m_pollfd.fd = listenSocket.m_fd;
			item2.m_pollfd.revents = 0;
			readFds.push_back(item2);

			// I/O ���� �̺�Ʈ�� ���� ������ ��ٸ��ϴ�.
			Poll(readFds.data(), (int)readFds.size(), 100);

			// readFds�� �����ؼ� �ʿ��� ó���� �մϴ�.
			int num = 0;
			for (auto readFd : readFds) {
				if (readFd.m_pollfd.revents != 0) {
					if (num == readFds.size() - 1) {	// ���� �����̸�
						// accept�� ó���Ѵ�.
						auto remoteClient = make_shared<RemoteClient>();

						// �̹� "Ŭ���̾�Ʈ ������ ������" �̺�Ʈ�� �� �����̹Ƿ� �׳� �̰��� ȣ���ص� �ȴ�.
						string ignore;
						listenSocket.Accept(remoteClient->tcpConnection, ignore);
						remoteClient->tcpConnection.SetNonblocking();

						// �� Ŭ���̾�Ʈ�� ��Ͽ� �߰�.
						remoteClients.insert({ remoteClient.get(), remoteClient });

						cout << "Client joined. There are " << remoteClients.size() << " connections.\n";
					}
					else {	// TCP ���� �����̸�
						// ���� �����͸� �״�� ȸ���Ѵ�.
						RemoteClient* remoteClient = readFdsToRemoteClients[num];

						int ec = remoteClient->tcpConnection.Receive();
						if (ec <= 0) {
							// ���� Ȥ�� ���� ����.
							// �ش� ������ ����
							remoteClient->tcpConnection.Close();
							remoteClients.erase(remoteClient);

							cout << "Client left. There are " << remoteClients.size() << " conncetions.\n";
						}
						else {
							// ���� ������ �״�� �۽��Ѵ�.

							// ��Ģ��ζ�� TCP ��Ʈ�� Ư���� �Ϻθ� �۽��ϰ� �����ϴ� ��쵵 ����ؾ� �ϳ�, ����
							remoteClient->tcpConnection.Send(remoteClient->tcpConnection.m_receiveBuffer, ec);
						}
					}
				}
				num++;
			}
		}

		// ����ڰ� ctrl-c�� ���� ������ �������� ���¶��
		listenSocket.Close();
		remoteClients.clear();

	}
	catch (Exception& e) {
		cout << "Exception! " << e.what() << "\n";
	}

	return 0;
}