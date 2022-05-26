//
// tcp_multi_server_iocp.cpp
// 
// multi-thread socket connect
// iocp and Overlapped I/O

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

// true�� ���α׷� ����
volatile bool stopWorking = false;

void ProcessSignalAction(int sig_number) {
	if (sig_number == SIGINT)
		stopWorking = true;
}

// TCP ���� ��ü
class RemoteClient {
public:
	shared_ptr<thread> thread;	// Ŭ���̾�Ʈ ó���� �ϴ� ������ 1��
	Socket tcpConnection;	// accept�� TCP ����

	RemoteClient() {}
	RemoteClient(SocketType socketType) : tcpConnection(socketType) {}
};

unordered_map<RemoteClient*, shared_ptr<RemoteClient>> remoteClients;

void ProcessClientLeave(shared_ptr<RemoteClient> remoteClient) {
	// ���� Ȥ�� ���� ����.
	// �ش� ������ ����.
	remoteClient->tcpConnection.Close();
	remoteClients.erase(remoteClient.get());

	cout << "Client left. There are " << remoteClients.size() << " connections.\n";
}

int main() {
	// ����ڰ� ctrl-c�� ������ ���η��� ����.
	signal(SIGINT, ProcessSignalAction);

	try {
		// IOCP�� ���� ���� Ŭ���̾�Ʈ�� �޾� ó���Ѵ�.

		// �� ���� Ŭ�� tcp 5555�� ������,
		// hello world �޽����� ����
		// ������ ���� �����͸� �״�� �����ִ� ���� ����
		// ���������� �� ó���� ����Ʈ ���� ���������� ���

		// TODO :
		// string ��ü�� ���ú��� ����, ������ ��� ��,
		// ���ɻ� �����Ǵ� �κ��� ���� ����

		// IOCP �غ�
		Iocp iocp(1);	// �� ������ �����带 1���� ���

		// TCP ������ �޴� ����
		Socket listenSocket(SocketType::Tcp);
		listenSocket.Bind(Endpoint("0.0.0.0", 5555));

		listenSocket.Listen();

		// IOCP�� �߰�
		iocp.Add(listenSocket, nullptr);

		// overlapped accept�� �ɾ�д�.
		auto remoteClientCandidate = make_shared<RemoteClient>(SocketType::Tcp);

		string errorText;
		if (!listenSocket.AcceptOverlapped(remoteClientCandidate->tcpConnection, errorText)
			&& WSAGetLastError() != ERROR_IO_PENDING) {
			throw Exception("Overlapped AcceptEx failed.");
		}
		listenSocket.m_isReadOverlapped = true;

		cout << "������ ���۵Ǿ����ϴ�.\n";
		cout << "ctrl-cŰ�� ������ ���α׷��� �����մϴ�.\n";

		// ���� ���ϰ� TCP ���� ���� ��ο� ���ؼ� I/O ����(avail) �̺�Ʈ�� ���� ������ ��ٸ���.
		// �׸��� ���� I/O ���� ���Ͽ� ���ؼ� ���� �Ѵ�.

		while (!stopWorking) {
			// I/O �Ϸ� �̺�Ʈ�� ���� ������ ��ٸ���.
			IocpEvents readEvents;
			iocp.Wait(readEvents, 100);

			// ���� �̺�Ʈ�� ���� ó��
			for (int i = 0; i < readEvents.m_eventCount; i++) {
				auto& readEvent = readEvents.m_events[i];
				if (readEvent.lpCompletionKey == 0)	{// ���� �����̸�
					listenSocket.m_isReadOverlapped = false;
					// �̹� accept�� �Ϸ�Ǿ���. ��������, Win32 AcceptEx ������ ���� ������ �۾��� ����.
					if (remoteClientCandidate->tcpConnection.UpdateAcceptContext(listenSocket) != 0) {
						// ���� ������ �ݾҴٸ� ������ �߻�. ���� ����
						listenSocket.Close();
					}
					else {	// ������ ���ٸ�
						shared_ptr<RemoteClient> remoteClient = remoteClientCandidate;

						// �� TCP ���ϵ� IOCP�� �߰�
						iocp.Add(remoteClient->tcpConnection, remoteClient.get());

						// overlapped ������ ���� �� �־�� �ϹǷ�, ���⼭ I/O ���� ��û�� �ɾ�д�.
						if (remoteClient->tcpConnection.ReceiveOverlapped() != 0
							&& WSAGetLastError() != ERROR_IO_PENDING) {
							// ����. ���� ����
							remoteClient->tcpConnection.Close();
						}
						else {
							// I/O�� �ɾ���. �ϷḦ ����ϴ� ���� ���·� �ٲ�
							remoteClient->tcpConnection.m_isReadOverlapped = true;

							// �� Ŭ���̾�Ʈ�� ��Ͽ� �߰�
							remoteClients.insert({ remoteClient.get(), remoteClient });

							cout << "Client joined. There are " << remoteClients.size() << " connections.\n";
						}

						// ����ؼ� ���� �ޱ⸦ �ؾ� �ϹǷ� �������ϵ� overlapped I/O�� ����.
						remoteClientCandidate = make_shared<RemoteClient>(SocketType::Tcp);
						string errorText;
						if (!listenSocket.AcceptOverlapped(remoteClientCandidate->tcpConnection, errorText)
							&& WSAGetLastError() != ERROR_IO_PENDING) {
							// ����. ���� ����
							listenSocket.Close();
						}
						else {
							// ���� ������ ������ ������ ��ٸ��� ���°� ��
							listenSocket.m_isReadOverlapped = true;
						}
					}
				}
				else {	// TCP ���� �����̸�
					// ���� �����͸� �״�� ȸ���Ѵ�.
					shared_ptr<RemoteClient> remoteClient = remoteClients[(RemoteClient*)readEvent.lpCompletionKey];

					if (readEvent.dwNumberOfBytesTransferred <= 0) {
						int a = 0;
					}

					if (remoteClient) {
						// �̹� ���ŵ� �����̴�. ���� �Ϸ�� ���� ���� ����.
						remoteClient->tcpConnection.m_isReadOverlapped = false;
						int ec = readEvent.dwNumberOfBytesTransferred;

						if (ec <= 0) {
							// ���� ����� 0. ��, TCP ������ ����
							// �������, ������ �� ����
							ProcessClientLeave(remoteClient);
						}
						else {
							// �̹� ���ŵ� �����̴�. ���� �Ϸ�� ���� ���� ����.
							char* echoData = remoteClient->tcpConnection.m_receiveBuffer;
							int echoDataLength = ec;

							// ��Ģ��ζ�� TCP ��Ʈ�� Ư���� �Ϻθ� �۽��ϰ� �����ϴ� ��쵵 ����ؾ� �ϳ�,
							// ���ظ� ���� ����.
							// ���⼭ overlapped �۽��� �ؾ� ������
							// �׳� ���ŷ �۽�.
							remoteClient->tcpConnection.Send(echoData, echoDataLength);

							// �ٽ� ������ �ޱ� ���� overlapped I/O�� �Ǵ�.
							if (remoteClient->tcpConnection.ReceiveOverlapped() != 0
								&& WSAGetLastError() != ERROR_IO_PENDING) {
								ProcessClientLeave(remoteClient);
							}
							else {
								// I/O�� �ɾ���. �ϷḦ ����ϴ� ���� ���·� �ٲ�
								remoteClient->tcpConnection.m_isReadOverlapped = true;
							}
						}
					}
				}
			}
		}

		// ����ڰ� ctrl-c�� ������ ������ �����������Ƿ�, ��� ���� ����
		// OS�� ��׶���� overlapped I/O�� �������̱� ������, overlapped I/O�� ��� �Ϸ�ǰ� �� �Ŀ� ������ �Ѵ�.
		// �ϷḦ ��� üũ�ϰ� �������� ����.
		listenSocket.Close();
		for (auto i : remoteClients) {
			i.second->tcpConnection.Close();
		}

		cout << "������ �����ϰ� �ֽ��ϴ�...\n";
		while (remoteClients.size() > 0 || listenSocket.m_isReadOverlapped) {
			// I/O completion�� ���� ������ RemoteClient�� �����Ѵ�.
			for (auto i = remoteClients.begin(); i != remoteClients.end();) {
				if (!i->second->tcpConnection.m_isReadOverlapped) {
					i = remoteClients.erase(i);
				}
				else
					i++;	// ��ٸ��� ���� �ͺ��� ����
			}

			// I/O completion�� �߻��ϸ� �� �̻� Overlapped I/O�� ���� ���� �����ϴ� �÷����� �Ѵ�.
			IocpEvents readEvents;
			iocp.Wait(readEvents, 100);

			// ���� �̺�Ʈ�� ���� ó��
			for (int i = 0; i < readEvents.m_eventCount; i++) {
				auto& readEvent = readEvents.m_events[i];
				if (readEvent.lpCompletionKey == 0) {	// ���� �����̸�
					listenSocket.m_isReadOverlapped = false;
				}
				else {
					shared_ptr<RemoteClient> remoteClient = remoteClients[(RemoteClient*)readEvent.lpCompletionKey];
					if (remoteClient) {
						remoteClient->tcpConnection.m_isReadOverlapped = false;
					}
				}
			}
		}
		
		cout << "���� ��.\n";
	}
	catch (Exception& e) {
		cout << "Exeption! " << e.what() << "\n";
	}

	return 0;
}