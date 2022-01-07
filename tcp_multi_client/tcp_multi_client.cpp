// tcp_multi_client
// multi-thread socket connect

#include "stdafx.h"
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <memory>
#include <iostream>

#include "../SocketLib/SocketLib.h"

using namespace std;

const int ClientNum = 10;

int main() {
	// �׽�Ʈ�� ���� ���μ��� �켱������ ����
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	using namespace std::chrono_literals;

	// �� ������� TCP ���� �ϳ��� ����
	// �� TCP ������ ������ ���� �� �����͸� ���������� ����
	// ������ ���� �����͸� �״�� Ŭ���̾�Ʈ���� ����
	// ��� Ŭ���̾�Ʈ�� ������ �ѷ��� ���

	recursive_mutex mutex;
	vector<shared_ptr<thread>> threads;
	int64_t totalReceivedBytes = 0;
	int connectedClientCount = 0;

	for (int i = 0; i < ClientNum; i++) {
		// TCP ������ �ϰ� �ۼ����ϴ� �����带 ����. ���� ������ ����.
		shared_ptr<thread> th = make_shared<thread>(
			[&connectedClientCount, &mutex, &totalReceivedBytes]() {

				try {

					Socket tcpSocket(SocketType::Tcp);
					tcpSocket.Bind(Endpoint::Any);
					tcpSocket.Connect(Endpoint("127.0.0.1", 5555));

					{
						lock_guard<recursive_mutex> lock(mutex);
						connectedClientCount++;
					}

					string receivedData;

					while (true) {
						const char* dataToSend = "hello world.";

						tcpSocket.Send(dataToSend, strlen(dataToSend) + 1);
						int receiveLength = tcpSocket.Receive();
						if (receiveLength <= 0) {
							// ���� ���� ����. ���� ����
							break;
						}
						lock_guard<recursive_mutex> lock(mutex);
						totalReceivedBytes += receiveLength;
					}
				}
				catch (exception& e) {
					lock_guard<recursive_mutex> lock(mutex);
					cout << "A TCP socket work failed: " << e.what() << endl;
				}

				lock_guard<recursive_mutex> lock(mutex);
				connectedClientCount--;
			});

		lock_guard<recursive_mutex> lock(mutex);
		threads.push_back(th);
	}

	// ���� ������� �� �ʸ��� �� ���ŷ��� ���
	while (true) {
		{
			lock_guard<recursive_mutex> lock(mutex);
			cout << "Total echoed bytes: " << (uint64_t)totalReceivedBytes <<
				", thread count: " << connectedClientCount << endl;
		}
		this_thread::sleep_for(2s);
	}

	return 0;
}