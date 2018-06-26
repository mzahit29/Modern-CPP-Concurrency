#include "stdafx.h"
#include "Examples.h"
#include <thread>
#include <mutex>
#include <iostream>
#include "thread_guard.h"

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




void Examples::handling_join_in_exceptions_run()
{
	cout << "\n\n HANDLING JOIN IN EXCEPTIONS EXAMPLES_________________________________" << endl;

	thread t1(func1);

	try
	{
		throw std::runtime_error("Run time exception in main thread");
		t1.join();	// This won't be called due to the exception thrown before it
	}
	catch (std::exception & e)
	{
		cout << e.what() << endl;
		t1.join();	// If you don't call t1.join() in catch then program will be aborted with error
	}

}

void Examples::handling_join_in_exceptions_with_thread_guard_run()
{
	cout << "\n\n HANDLING JOIN WITH THREAD GUARD IN EXCEPTIONS EXAMPLES_________________________________" << endl;
	// Note that during stack unwind the local objects are destructed in REVERSE ORDER. This means tg will be destructed
	// first, then t1. If t1 was destructed first then this code would have failed.
	// i.e. If the following code was possible (it is not possible since we deleted the copy assignment operator)
	// then t2 would be destructed before tg2. So when tg2 is destructed it would try to call join on t2 which would
	// give run time error.
	/*thread_guard tg2;
	thread t2{ func1 };
	tg2 = t2;*/
	thread t1{ func1 };
	thread_guard tg{ t1 };


	// Now we don't need to explicitly call the join of t1. It will be called when this function's stack is unwinded.
	try
	{
		throw std::runtime_error("Run time exception in main thread");
		t1.join();	// This won't be called due to the exception thrown before it but no need to call it either
	}
	catch (std::exception & e)
	{
		cout << e.what() << endl;
	}
}
