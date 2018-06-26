#include "stdafx.h"
#include "Examples.h"
#include <thread>
#include <mutex>
#include <iostream>
#include "thread_guard.h"
#include <vector>
#include <algorithm>
#include <numeric>
#include <list>

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

template <typename T>
T add(const T& x, const T& y)
{
	cout << x << " + " << y << " = " << x + y << endl;
	return x + y;
}

void monitor_variable(const int & x)
{
	while (true)
	{
		cout << "Value of x is " << x << endl;
		if (x == 15) { cout << "x value is set to 15 from another thread, exiting loop " << endl; break; }
		this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Examples::passing_parameters_to_thread()
{
	cout << "\n\n PASSING PARAMETERS TO THREAD_________________________________" << endl;
	thread t{ add<float>, 3.f, 4.f };

	t.join();

	int x{ 10 };
	thread t2{ monitor_variable, std::ref(x) }; // Remember you have to use std::ref(x) to pass x as reference to the function monitor_variable(int & x)
	this_thread::sleep_for(std::chrono::milliseconds(1000));
	cout << "Setting x to 15 in main thread" << endl;
	x = 15; // This will end the while loop in t2 and it will exit

	t2.join();

}

void f2(int& x); // forward declaration
void f1()
{
	int x = 10;
	thread t{ f2, std::ref(x) };
	t.detach();
	this_thread::sleep_for(chrono::milliseconds(500));
}
void f2(int& x)
{
	int i = 0;
	while(true)
	{
		// This will cause runtime exception when thread that spawned this thread exits before
		// x is created inside the parent thread and this child thread is referencing it. If parent
		// exits before this will be a read access violation
		//cout << "x is : " << x << endl;
		this_thread::sleep_for(chrono::milliseconds(100));
		if (++i > 20) break;
	}
}
void Examples::passing_parameters_by_ref_problem_run()
{
	cout << "\n\n PASSING PARAMETERS TO THREAD BY REFERENCE CAUSING READ ACCESS VIOLATION_________________________________" << endl;
	thread t1{ f1 };
	t1.join();
}

void g()
{
	cout << "g()" << endl;
}
void h()
{
	cout << "h()" << endl;
}
void Examples::passing_thread_ownership_run()
{
	cout << "\n\n PASSING THREAD OWNERSHIP_________________________________" << endl;
	thread t1{ g };
	//thread t2 = t1; // Compile Error: Copy constructer is deleted
	thread t2 = std::move(t1);  // Allowed: std::move turns t1 into a r-value reference, this triggers the move constructor which is not deleted for thread class
	// Now t1 is nullptr
	t1 = thread{ h };  // Right hand side is a temporary (r-value). Again this will trigger move constructor, which is allowed.
	//t1 = thread{ h };  // This will cause program to crash, because t1's previously pointed to thread is lost and it will not be able to join.

	if (t1.joinable())
	{
		cout << "t1 is joinable" << endl;
		t1.join();
	}
	if (t2.joinable())
	{
		cout << "t2 is joinable" << endl;
		t2.join();
	}
}

void g1()
{
	cout << "Thread id: " << this_thread::get_id() << endl;
}
void Examples::useful_thread_functions_run()
{
	cout << "\n\nUSEFUL THREAD FUNCTIONS_________________________________" << endl;
	thread t1{ g1 };
	cout << "t1's id seen from main thread: " << t1.get_id() << endl;
	t1.join();
	cout << "t1's id seen from main thread after t1.join(): " << t1.get_id() << endl;

	// Default constructed thread is non-joinable so it will always print 0 as id.
	thread t2;
	cout << "t2's default constructed so it is non-joinable. Its id seen from main thread: " << t2.get_id() << endl;

	cout << "\n\nAllowed max number of parallel threads: " << thread::hardware_concurrency() << endl;

}


void accumulate_block(vector<int>::iterator begin, vector<int>::iterator end, int& result)
{
	result = std::accumulate(begin, end, 0);
	cout << this_thread::get_id() << " - result: " << result << endl;
}
void Examples::parallel_accumulate_run()
{
	// List to find the sum
	int num_of_elements = 1000;
	vector<int> list(num_of_elements);
	for(int i = 0; i < num_of_elements; ++i)
	{
		list[i] = 1;
	}

	int min_load = 100;
	// Find the necessary num of threads
	int distance = std::distance(list.begin(), list.end());
	int max_threads = distance / min_load;
	int allowed_thread = thread::hardware_concurrency();
	int running_thread_count = std::min(max_threads, allowed_thread);

	vector<thread> threads(running_thread_count - 1);
	vector<int> results(running_thread_count);

	// Send particular block to separate thread to accumulate
	vector<int>::iterator block_start = list.begin();
	vector<int>::iterator block_end = list.end();
	int block_size = distance / running_thread_count;
	for (int i = 0; i < running_thread_count - 1; ++i)
	{
		block_end = block_start;
		std::advance(block_end, block_size);
		threads[i] = thread{ accumulate_block, block_start, block_end, std::ref(results[i]) };
		block_start = block_end;
	}

	// Accumulate the last block in main thread
	accumulate_block(block_start, list.end(), results[running_thread_count - 1]);

	// Join all the threads
	for_each(threads.begin(), threads.end(), std::mem_fn(&thread::join));

	int final_sum = std::accumulate(results.begin(), results.end(), 0);
	cout << "Sum of values in list: " << final_sum << endl;
}

mutex m;
void add_to_list(int const & x, list<int> & my_list)
{
	// RAII in action: unlock() on destruction of lock_guard<T> object.
	lock_guard<mutex> lg(m);
	cout << "[" << this_thread::get_id() << "]" << "Adding " << x << " to list" << endl;
	my_list.push_back(x);
}
void list_size(const list<int> & my_list)
{
	lock_guard<mutex> lg(m);
	cout << "[" << this_thread::get_id() << "]" << "Size of list is : " << my_list.size() << endl;
}


void Examples::lock_guard_run()
{
	list<int> my_list;

	vector<thread> threads(4);
	threads[0] = thread{ add_to_list, 10, std::ref(my_list) };
	threads[1] = thread{ list_size, std::ref(my_list) };
	threads[2] = thread{ add_to_list, 10, std::ref(my_list) };
	threads[3] = thread{ list_size, std::ref(my_list) };

	for(auto& t : threads)
	{
		t.join();
	}

}
