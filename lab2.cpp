#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <queue>
#include <algorithm>
#include <stack>
#include <numeric>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <iomanip>

using namespace std;

#define ll long long
#define ull unsigned long long
#define pb push_back
#define pp pop_back
#define mp make_pair
#define all(x) x.begin(),x.end()
#define fr(n,i) for(int i=0;i<n;++i)
#define FR(n,i) for(int i=1;i<=n;++i)
#define fr2(n,i) for(int i=0;i<n;i+=2)
#define INFINITY 1e18 + 3
#define MOD 998244353
#define BORDER 1024
const int MAX = 100005;

bool parity(int n) { return !(n & 1); }

bool is2(int n) { return n & (n - 1) == 0; }

template<typename T>
class ThreadVector {
private:
	vector<vector<T>> arr;
	vector<T> arr_atomic;
	vector<thread> threads;
	unordered_map<thread::id, double> tm;
	mutex lock;
	int _i, cnt;
	atomic<int> __i = 0;

	void funM(vector<vector<T>> & v)
	{
		for (;;) {
			lock.lock();
			_i++;
			if (_i >= BORDER * BORDER) {
				lock.unlock();
				break;
			}
			arr_atomic[_i]++;
			lock.unlock();
		}
	}

	void funA(vector<T> & v) {
		for (;;) {
			auto tmp = __i.fetch_add(1, memory_order_relaxed);
			if (tmp >= BORDER * BORDER)
				return;
			v[tmp]++;
		}
	}

public:
	ThreadVector(vector<vector<T>> && v, int n) : arr(move(v)), _i(-1), __i(0), cnt(n) { }
	ThreadVector(const vector<vector<T>> & v, int n) : arr(v), _i(-1), __i(0), cnt(n) { }
	ThreadVector(vector<T> && v, int n) : arr_atomic(move(v)), _i(-1), __i(0), cnt(n) { }
	ThreadVector(const vector<T> & v, int n) : arr_atomic(v), _i(-1), __i(0), cnt(n) {  }

	void startThreadsM()
	{
		auto t = [&](vector<vector<T>> & v) { funM(v); };
		auto start = chrono::high_resolution_clock::now();
		fr(cnt, i) {
			thread thr(t, ref(arr));
			threads.pb(move(thr));
		}

		fr(cnt, j)
			if (threads[j].joinable())
				threads[j].join();
		auto stop = chrono::high_resolution_clock::now();
		auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
		cout << duration.count() << endl;
	}

	void startThreadsALinear()
	{
		auto t = [&](vector<T> & v) { funA(v); };
		auto start = chrono::high_resolution_clock::now();
		fr(cnt, i) {
			thread thr(t, ref(arr_atomic));
			threads.pb(move(thr));
		}

		fr(cnt, j)
			if (threads[j].joinable())
				threads[j].join();
		auto stop = chrono::high_resolution_clock::now();
		auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);
		cout << duration.count() << endl;
	}

	void printVector()
	{
		fr(arr.size(), i) {
			fr(arr[0].size(), j)
				cout << arr[i][j];
			cout << endl;
		}
	}

	void printLinear() {
		fr(arr_atomic.size(), i)
			cout << arr_atomic[i] << " ";
		cout << endl;
	}

	void printTime()
	{
		for (auto i = tm.begin(); i != tm.end(); ++i)
			cout << i->first << " " << i->second << endl;
	}
};

template<typename T>
class ThreadQueueM {
private:
	vector<int> q;
	int p_num, t_num, c_num, size, ind = 0, ind1 = 0;
	mutex ph, p, lockph, lockp;
	condition_variable conditionPH, conditionP;
	atomic<int> curSizePH = 0, curSizeP = 0;
	atomic<int> cur = 0;
	int sum = 0;
	bool flg;

public:
	ThreadQueueM(int size, int t_num, int p_num, int c_num) : t_num(t_num), p_num(p_num), c_num(c_num), size(size)
	{
		q.resize(size);
		fr(q.size(), i)
			q[i] = 0;
		flg = false;
	}

	void pop()
	{
		for (;;) {
			unique_lock<mutex> lock(lockp);
			if (curSizeP.load() >= c_num * t_num) {
				conditionPH.notify_all();
				return;
			}
			while (cur.load() <= 0) {
				if (curSizeP.load() >= c_num * t_num) {
					conditionPH.notify_all();
					return;
				}
				conditionP.wait(lock);
			}
			if (curSizeP.load() >= c_num * t_num) {
				conditionPH.notify_all();
				return;
			}
			if (!q[ind % size])
				this_thread::sleep_for(chrono::milliseconds(1));
			sum += q[ind % size];
			q[ind % size] = 0;
			cur--;
			conditionPH.notify_all();
			ind++;
			ind %= size;
			curSizeP++;
		}
	}

	void push()
	{
		for (;;) {
			unique_lock<mutex> lock(lockph);
			if (curSizePH.load() >= p_num * t_num) {
				conditionP.notify_all();
				conditionPH.notify_all();
				return;
			}
			while (cur.load() >= 4) {
				if (curSizePH.load() >= p_num * t_num) {
					conditionPH.notify_all();
					return;
				}
				conditionPH.wait(lock);
			}
			if (curSizePH.load() >= p_num * t_num) {
				conditionPH.notify_all();
				return;
			}
			if (q[ind1 % size])
				this_thread::sleep_for(chrono::milliseconds(1));
			q[ind1 % size] = 1;
			cur++;
			conditionP.notify_all();
			ind1++;
			ind1 %= size;
			curSizePH++;
		}
	}

	void startThreads()
	{
		vector<thread> threadsPH, threadsP;
		auto ph = [&]() { push(); };
		auto p = [&]() { pop(); };

		fr(p_num, i) {
			thread thr(ph);
			threadsPH.pb(move(thr));
		}

		fr(c_num, i) {
			thread thr(p);
			threadsP.pb(move(thr));
		}

		fr(p_num, i)
			if (threadsPH[i].joinable())
				threadsPH[i].join();
		fr(c_num, i)
			if (threadsP[i].joinable())
				threadsP[i].join();
	}

	int returnSum() {
		return sum;
	}
};

template<typename T>
class ThreadQueueA
{
private:
	vector<T> q;
	int p_num, t_num, c_num, size;
	atomic<int> sum, curSizePH = 0, curSizeP = 0;
	mutex lock, lock1, locktest1;
	atomic<T> ind1 = 0, ind2 = 0;

public:
	ThreadQueueA(int size, int t_num, int p_num, int c_num) : t_num(t_num), p_num(p_num), c_num(c_num), size(size)
	{
		p_num = min(p_num, c_num);
		c_num = min(p_num, c_num);
		q.resize(size);
	}

	void pop()
	{
		for (;;) {
			if (curSizeP.load() >= c_num * t_num)
				return;
			T i = ind2.load(memory_order_relaxed);
			while (!ind2.compare_exchange_weak(
				i,
				(ind2 + 1) % q.size(),
				std::memory_order_release,
				std::memory_order_relaxed));
			{
				unique_lock<mutex> un(locktest1);
				if (!q[(i % q.size())])
					continue;
				if (curSizeP.load() >= c_num * t_num)
					return;
				sum += q[(i % q.size())];
				q[(i % q.size())] = 0;
				curSizeP++;
			}
		}
	}

	void push()
	{
		for (;;) {
			if (curSizePH.load() >= p_num * t_num)
				return;
			T i = ind1.load(memory_order_relaxed);
			while (!ind1.compare_exchange_weak(
				i,
				(ind1 + 1) % q.size(),
				std::memory_order_release,
				std::memory_order_relaxed));
			{
				unique_lock<mutex> un(locktest1);
				if (q[(i % q.size())])
					continue;
				if (curSizePH.load() >= p_num * t_num)
					return;
				q[(i % q.size())]++;
				curSizePH++;
			}
		}
	}

	void print() {
		int ans = 0;
		fr(q.size(), i)
			cout << q[i] << " ", ans += q[i];
		cout << endl;
		cout << ans << endl;
	}

	void startThreads()
	{
		vector<thread> threadsPH, threadsP;
		auto ph = [&]() { push(); };
		auto p = [&]() { pop(); };

		fr(p_num, i) {
			thread thr(ph);
			threadsPH.pb(move(thr));
		}

		fr(c_num, i) {
			thread thr(p);
			threadsP.pb(move(thr));
		}

		fr(p_num, i)
			if (threadsPH[i].joinable())
				threadsPH[i].join();

		fr(c_num, i)
			if (threadsP[i].joinable())
				threadsP[i].join();
	}

	int returnSum() {
		return sum;
	}
};

template<typename T>
class ThreadQueue
{
private:
	queue<T> q;
	mutex lock1, lock2, lock3;
	int p_num, t_num, c_num, i = 0, j = 0;
	ll sum = 0;

public:
	ThreadQueue(int t_num, int p_num, int c_num) : t_num(t_num), p_num(p_num), c_num(c_num) {}

	void pop()
	{
		lock2.lock();
		while (i >= j) {
			if (j >= p_num && q.empty()) {
				lock2.unlock();
				return;
			}
		}
		fr(t_num, k) {
			lock3.lock();
			sum += q.front();
			q.pop();
			lock3.unlock();
		}
		i++;
		lock2.unlock();
	}

	void push()
	{
		lock1.lock();
		fr(t_num, k) {
			lock3.lock();
			q.push(1);
			lock3.unlock();
		}
		j++;
		lock1.unlock();
	}

	void startThreads()
	{
		vector<thread> threadsPH, threadsP;
		auto ph = [&]() { push(); };
		auto p = [&]() { pop(); };

		fr(p_num, i) {
			thread thr(ph);
			threadsPH.pb(move(thr));
		}

		fr(c_num, i) {
			thread thr(p);
			threadsP.pb(move(thr));
		}

		fr(p_num, i)
			if (threadsPH[i].joinable())
				threadsPH[i].join();
		fr(c_num, i)
			if (threadsP[i].joinable())
				threadsP[i].join();
	}

	ll returnSum() {
		return sum;
	}
};

int main() {
	ios_base::sync_with_stdio(false);
	cin.tie(0);
	cout.tie(0);

	int DEBUG = 4;

	if (DEBUG == 1) {
		int n = BORDER, m = BORDER, k;
		cin >> k;
		vector<int> tm(n * n);

		ThreadVector<int> vc(tm, k); // <1 for atomic and <10 for lock
		vc.startThreadsM();
		ThreadVector<int> c(tm, k);
		c.startThreadsALinear();
		//vc.printLinear();
		//vc.printTime();
	}

	if (DEBUG == 2) {
		ThreadQueueM<int> q(4, 1024 * 100, 4, 4); //1.10
		q.startThreads();
		cout << q.returnSum() << endl;
	}

	if (DEBUG == 3) {
		int val = 1024;
		for (int i = 0; i < 10000; ++i) {
			ThreadQueue<uint8_t> q(val, 4, 4);//1.20
			q.startThreads();
		}
	}

	if (DEBUG == 4) {
		for (int j = 1; j < 6; ++j) {
			int cnt1 = 0, cnt2 = 0, cnt3 = 0;
			int val = 1021 * j;
			cout << val << endl;
			for (int i = 0; i < 50; ++i) {
				ThreadQueueA<int> q(1, val, 1, 1);//1.37
				q.startThreads();
				if (q.returnSum() != val) {
					cout << "err!" << endl;
					cout << q.returnSum() << endl;
					cnt1++;
				}
			}
			if (!cnt1)
				cout << "success 1" << endl;
			else
				cout << "fail 1" << endl;
			for (int i = 0; i < 50; ++i) {
				ThreadQueueA<int> q(4, val, 2, 2);//1.37
				q.startThreads();
				if (q.returnSum() != val * 2) {
					cout << "err!" << endl;
					cout << q.returnSum() << endl;
					cnt2++;
				}
			}
			if (!cnt2)
				cout << "success 2" << endl;
			else
				cout << "fail 2" << endl;
			for (int i = 0; i < 50; ++i) {
				ThreadQueueA<int> q(16, val, 4, 4);//1.37
				q.startThreads();
				if (q.returnSum() != val * 4) {
					cout << "err!" << endl;
					cout << q.returnSum() << endl;
					cnt3++;
				}
			}
			if (!cnt3)
				cout << "success 3" << endl;
			else
				cout << "fail 3" << endl;
		}
	}

	cout << "ended!" << endl;

	return 0;
}
