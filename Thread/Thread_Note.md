#### 스레드 작업 (람다식)  
#### thread t1( \[ \] ( ) {  
####	작업 내용  
#### });
<br>

**Critical Section**  
#### 크리티컬 색션 객체 생성  
#### CRITICAL_SECTION m_critSec;  

#### 객체 초기화  
#### InitializeCriticalSectionEx(&m_critSec, 0, 0);

#### 객체 삭제  
#### DeleteCriticalSection(&m_critSec);

#### 락 Lock()  
#### EnterCriticalSection(&m_critSec);

#### 언락 Unlock()  
#### LeaveCriticalSection(&m_critSec);
<br>

**Event**
#### 이벤트 객체 생성
#### Event event1;

#### 이벤트 생성
#### CreateEvent();

#### 이벤트 삭제
#### CloseHandle();

#### 이벤트 대기
#### WaitForSingleObject();

#### 이벤트 신호 전송
#### SetEvent();
#### -> 모든 스레드가 깨어나고 자동으로 상태가 0으로 바뀜, 수동 이벤트의 경우, 모든 스레드의 상태 값이 1에서 유지함
#### PulseEvent();
#### -> 수동 이벤트라고 해도 모든 스레드가 깨어났다가 다시 0으로 상태가 바뀜
<br>

**Semaphore**  
#### 세마포 생성
#### CreateSemaphore();

#### 세마포 대기
#### WaitForSingleObject();

#### 세마포 자원 해제
#### ReleaseSemaphore();

#### 세마포 삭제
#### CloseHandle();