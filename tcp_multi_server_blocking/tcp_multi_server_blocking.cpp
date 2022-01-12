//
// tcp_many_server_blocking.cpp
// 
// multi-thread socket connect

#include "stdafx.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <signal.h>
#include <memory>
#include <vector>
#include <deque>
#include <mutex>

#include "../SocketLib/SocketLib.h"

using namespace std;

// TCP ���� ������ ��ü
struct RemoteClient {
	shared_ptr<thread> thread;	// Ŭ���̾�Ʈ ó���� �ϴ� ������ 1��
	Socket tcpConnection;	// accept�� TCP ����
};

unordered_map<RemoteClient*, shared_ptr<RemoteClient>> remoteClients;	// ���� ���� TCP ���� ��ü��. key�� RemoteClient�� �����Ͱ� ��ü��.
deque<shared_ptr<RemoteClient>> remoteClientDeathNote;	// ���⿡ �� RemoteClient�� �� �ı��ȴ�.
Semaphore mainThreadWorkCount;	// ���⿡ +1�� �߻��ϸ� ���� �� ���� �ִٴ� ���̴�. ���� �����尡 ����� �� ���� �Ѵ�.

// �� ������ ��ȣ�ϴ� mutex. ���� ������� TCP ���� �ޱ⸦ ó���ϴ� �����尡 �����ϱ� ������ �ʿ��ϴ�.
recursive_mutex remoteClientsMutex;
recursive_mutex consoleMutex;	// �ܼ� ����� ���� �����尡 �ϸ� ���� �� �ִ�. �׷��� ó���� �Ϸķ� �ټ�����.

volatile bool stopWorking = false;

shared_ptr<Socket> listenSocket;	// ������ ����

void RemoteClientThread(shared_ptr<RemoteClient> remoteClient);
void ListenSocketThread();

void ProcessSignalAction(int sig_number) {
	if (sig_number == SIGINT) {
		stopWorking = true;
		mainThreadWorkCount.Notify();
	}
}

int main() {

	// �� ���α׷��� ���� �����尡 CPU �ڿ��� �ܶ� ���� ���̴�. �� ������ ��ǻ�Ͱ� ������ �� ���� �ִ�.
	// ���� ���μ��� �켱������ ���߾ ������ �������� �ʰ� ���ش�.
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	try {
		// ����ڰ� ctrl-c�� ������ ���η����� �����ϰ� ����ϴ�.
		signal(SIGINT, ProcessSignalAction);

		// Ŭ�� ���� �� ��ŭ �����带 �����ϴ� ���ŷ ���� ����̴�.

		// �� ���� Ŭ�� tcp 5555�� ������,
		// hello world �޽����� ����
		// ������ ���� �����͸� �״�� �����ִ� ���� ����
		// ���������� �� ó���� ����Ʈ ���� ���������� ���

		// TCP ������ �޴� ����
		listenSocket = make_shared<Socket>(SocketType::Tcp);
		listenSocket->Bind(Endpoint("0.0.0.0", 5555));
		listenSocket->Listen();

		cout << "������ ���۵Ǿ����ϴ�.\n";
		cout << "ctrl-c�� ������ ���α׷��� �����մϴ�.\n";

		// ������ ������ ���� �����带 �����Ѵ�.
		thread tcpListenThread(ListenSocketThread);

		while (!stopWorking) {
			// ���� �����忡�� �� ���� ���� ������ ��ٸ���.
			mainThreadWorkCount.Wait();

			// ����� ó��
			lock_guard<recursive_mutex> lock(remoteClientsMutex);

			while (!remoteClientDeathNote.empty()) {
				auto remoteClientToDelete = remoteClientDeathNote.front();
				remoteClientToDelete->tcpConnection.Close();
				remoteClientToDelete->thread->join();
				remoteClients.erase(remoteClientToDelete.get());	// Ŭ���̾�Ʈ ��Ͽ��� �����Ѵ�.
				remoteClientDeathNote.pop_front();	// socket close�� �� �������� �̷������.

				lock_guard<recursive_mutex> lock(consoleMutex);
				cout << "Client left. There are " << remoteClients.size() << " connections.\n";
			}
		}

		// ���η����� ������. ���α׷��� ���� �ؾ� �ϴ� ��Ȳ�̴�.

		// ������ �ִ� ��� Ŭ���̾�Ʈ ���ϵ� �ݾ� ������.
		listenSocket->Close();
		{
			lock_guard<recursive_mutex> lock(remoteClientsMutex);
			for (auto i : remoteClients) {
				// ��� �����带 �����Ų��.
				i.second->tcpConnection.Close();
				i.second->thread->join();
			}
		}

		// ��� �����带 �����Ų��.
		tcpListenThread.join();

		// ��������� ��� �����Ҷ����� ��ٸ��� ó���� ���Եȴ�.
		remoteClients.clear();
	}
	catch (Exception& e) {
		cout << "Exception! " << e.what() << "\n";
	}

	return 0;
}

// TCP ���� ������ �޴� ó���� �ϴ� ������
void ListenSocketThread() {
	while (!stopWorking) {
		auto remoteClient = make_shared<RemoteClient>();
		// TCP ������ ������ ��ٸ���.
		string errorText;
		if (listenSocket->Accept(remoteClient->tcpConnection, errorText) == 0) {
			// TCP ������ ���Դ�.
			// RemoteClient ��ü�� ������.
			{
				lock_guard<recursive_mutex> lock(remoteClientsMutex);
				remoteClients.insert({ remoteClient.get(), remoteClient });

				// TCP Ŭ��-���� ����� ���� �����带 �ϳ� ���������.
				// TCP ���� �ϳ��� ������ �ϳ���.
				remoteClient->thread = make_shared<thread>(RemoteClientThread, remoteClient);
			}

			// �ܼ� ���
			{
				lock_guard<recursive_mutex> lock(consoleMutex);
				cout << "Client joined. There are " << remoteClients.size() << " connections.\n";
			}
		}
		else {
			// accept�� �����ߴ�. ������ �����带 ��������.
			break;
		}
	}
}

// �̹� ���� TCP ������ ó���ϴ� ������
void RemoteClientThread(shared_ptr<RemoteClient> remoteClient) {
	while (!stopWorking) {
		// ���� �����͸� �״�� ȸ���Ѵ�.
		int ec = remoteClient->tcpConnection.Receive();
		if (ec <= 0) {
			if (ec <= 0) {
				// ���� Ȥ�� ���� �����.
				break;
			}
		}

		// ��Ģ��ζ�� TCP ��Ʈ�� Ư���� �Ϻθ� �۽��ϰ� �����ϴ� ��쵵 ����ؾ� �ϳ�,
		// ������ ������ ���ذ� �켱�̹Ƿ�, �����ϵ��� �Ѵ�.
		remoteClient->tcpConnection.Send(remoteClient->tcpConnection.m_receiveBuffer, ec);
	}

	// ������ ������. �� RemoteClient�� ����Ǿ�� ���� ����ο� ������.
	remoteClient->tcpConnection.Close();
	lock_guard<recursive_mutex> lock(remoteClientsMutex);
	remoteClientDeathNote.push_back(remoteClient);
	mainThreadWorkCount.Notify();
}