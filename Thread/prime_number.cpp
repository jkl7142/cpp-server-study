#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>

using namespace std;

const int MaxCount = 150000;
const int ThreadCount = 4;

bool IsPrimeNumber(int number) {
	if (number == 1)
		return false;
	if (number == 2 || number == 3)
		return true;
	for (int i = 2; i < number - 1; i++) {
		if ((number % i) == 0)
			return false;
	}
	return true;
}

void PrintNumbers(const vector<int>& primes) {
	for (int v : primes) {
		cout << v << endl;
	}
}

int main() {
	int num = 1;
	recursive_mutex num_mutex;

	vector<int> primes;
	recursive_mutex primes_mutex;

	auto t0 = chrono::system_clock::now();

	// 1. 싱글스레드
	/*
	for (int i = 1; i <= MaxCount; i++) {
		if (IsPrimeNumber(i)) {
			primes.push_back(i);
		}
	}
	*/

	// 2. 멀티스레드 기본개념
	/*
	vector<shared_ptr<thread>> threads;

	for (int i = 0; i < ThreadCount; i++) {
		shared_ptr<thread> thread1(new thread([&]() {
			// 각 스레드의 메인 함수
			while (true) {
				int n;
				n = num;
				num++;

				if (n >= MaxCount)
					break;
				if (IsPrimeNumber(n)) {
					primes.push_back(n);
				}
			}
		}));
		// 스레드 객체를 일단 갖고 있는다.
		threads.push_back(thread1);
	}

	for (auto thread : threads) {
		thread->join();
	}
	*/

	// 3. 멀티스레드 + 뮤텍스
	vector<shared_ptr<thread>> threads;

	for (int i = 0; i < ThreadCount; i++) {
		shared_ptr<thread> thread1(new thread([&]() {
			while (true) {
				int n;
				{
					lock_guard<recursive_mutex> num_lock(num_mutex);
					n = num;
					num++;
				}

				if (n >= MaxCount)
					break;

				if (IsPrimeNumber(n)) {
					lock_guard<recursive_mutex> primes_lock(primes_mutex);
					primes.push_back(n);
				}
			}
			}));

		threads.push_back(thread1);
	}

	for (auto thread : threads) {
		thread->join();
	}

	auto t1 = chrono::system_clock::now();
	auto duration = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();
	cout << "Took " << duration << " milliseconds." << endl;

}