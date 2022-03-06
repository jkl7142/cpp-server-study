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
	// 테스트를 위해 프로세스 우선순위를 낮춤
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	using namespace std::chrono_literals;

	// 각 스레드는 TCP 연결 하나를 수행
	// 각 TCP 연결은 서버에 접속 후 데이터를 지속적으로 보냄
	// 서버는 받은 데이터를 그대로 클라이언트에게 보냄
	// 모든 클라이언트가 수신한 총량을 출력

	recursive_mutex mutex;
	vector<shared_ptr<thread>> threads;
	int64_t totalReceivedBytes = 0;
	int connectedClientCount = 0;

	for (int i = 0; i < ClientNum; i++) {
		// TCP 연결을 하고 송수신하는 스레드를 생성. 무한 루프를 돈다.
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
							// 소켓 연결 오류. 루프 종료
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

	// 메인 스레드는 매 초마다 총 수신량을 출력
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