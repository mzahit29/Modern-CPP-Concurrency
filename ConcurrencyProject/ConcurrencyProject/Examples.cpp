#include "stdafx.h"
#include "Examples.h"
#include <thread>
#include <mutex>
#include <iostream>

using namespace std;


class Lock
{
	static mutex m;
public:
	Lock() { m.lock(); }
	~Lock() { m.unlock(); }
};

mutex Lock::m;

void func1()
{
	Lock lock;
	cout << "Hello from func1()" << endl;
}

class MyFuncCls
{
public:
	void operator()()
	{
		Lock lock;
		cout << "Hello from MyFuncCls::operator()" << endl;
	}
};

void Examples::join_and_detach_run()
{
	cout << "\n\n JOIN AND DETACH EXAMPLES_________________________________" << endl;

	std::thread t1(func1);
	MyFuncCls mc;
	thread t2(mc);

	thread t3([]()
	{
		cout << "Hello from lambda function" << endl;
		for (int i = 0; i < 4; ++i)
		{
			cout << "Inside thread t3, i: " << i << endl;
			this_thread::sleep_for(chrono::milliseconds(1000));
		}
	});

	// If thread object is destroyed before calling join or deatch on it, then program will call std::terminate
	// i.e. if you remove t4p->join() line then program will crash.
	thread * t4p = new thread(func1);
	t4p->join(); delete t4p; t4p = nullptr;

	if (t1.joinable()) cout << "t1 thread is joinable" << endl;
	else cout << "t1 thread is non-joinable" << endl;
	t1.join();
	// Sine t1 already joined it is non-joinable
	if (t1.joinable()) cout << "t1 thread is joinable" << endl;
	else cout << "t1 thread is non-joinable" << endl;
	//t1.join();  // ERROR: join or detach has to be called only once.
	t2.join();
	t3.detach();  // main thread is not blocked until t3 finishes. t3 runs independently and main thread continues.
				  // If main thread happens to finish before t3 then t3 will not be able to finish its execution
}
