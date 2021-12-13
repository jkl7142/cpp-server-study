
// ������ �۾� (���ٽ�)
thread t1([]() {
	�۾� ����
});


** Critical Section **
// ũ��Ƽ�� ���� ��ü ����
CRITICAL_SECTION m_critSec;

// ��ü �ʱ�ȭ
InitializeCriticalSectionEx(&m_critSec, 0, 0);

// ��ü ����
DeleteCriticalSection(&m_critSec);

// �� Lock()
EnterCriticalSection(&m_critSec);

// ��� Unlock()
LeaveCriticalSection(&m_critSec);

** Event **
// �̺�Ʈ ��ü ����
Event event1;

// �̺�Ʈ ����
CreateEvent();

// �̺�Ʈ ����
CloseHandle();

// �̺�Ʈ ���
WaitForSingleObject();

// �̺�Ʈ ��ȣ ����
SetEvent(); // ��� �����尡 ����� �ڵ����� ���°� 0���� �ٲ�, ���� �̺�Ʈ�� ���, ��� �������� �̺�Ʈ ���� ���� 1���� ������
PulseEvent(); // ���� �̺�Ʈ��� �ص� ��� �����尡 ����ٰ� �ٽ� 0���� ���°� �ٲ�

** Semaphore **
// ������ ����
CreateSemaphore();

// ������ ���
WaitForSingleObject();

// ������ �ڿ� ����
ReleaseSemaphore();

// ������ ����
CloseHandle();
