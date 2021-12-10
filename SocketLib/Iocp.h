#pragma once

class Socket;
class IocpEvents;

// I/O Completion Port ��ü
class Iocp {
public:
	// 1ȸ�� GetQueuedCompletionStatus�� �ִ��� ������ �� �ִ� �۾� ����
	static const int MaxEventCount = 1000;

	Iocp(int threadCount);
	~Iocp();

	void Add(Socket& socket, void* userPtr);

	HANDLE m_hIocp;
	int m_threadCount;	// IOCP ������ �� ���� �߰��� ��� ���Ǵ� ��
	void Wait(IocpEvents& output, int timeoutMs);
};

// IOCP�� GetQueuedCompletionStatus�� ���� I/O �Ϸ��ȣ��
class IocpEvents {
public:
	// GetQueuedCompletionStatus�� ������ �̺�Ʈ��
	OVERLAPPED_ENTRY m_events[Iocp::MaxEventCount];
	int m_eventCount;
};