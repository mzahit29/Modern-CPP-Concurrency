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
#include <future>

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
	cout << "\n\nPARALLEL ACCUMULATE_________________________________" << endl;
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
	// RAII in action: unlock() on destruction of lock_guard<T> object.
	lock_guard<mutex> lg(m);
	cout << "[" << this_thread::get_id() << "]" << "Size of list is : " << my_list.size() << endl;
}


void Examples::lock_guard_run()
{
	cout << "\n\nLOCK GUARD_________________________________" << endl;
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

int total_distance{ 10 };
int distance_covered{ 0 };
std::condition_variable cv;
std::mutex m2;

void keep_moving()
{
	while (true)
	{
		this_thread::sleep_for(chrono::milliseconds(20));
		++distance_covered;
		cout << "I am the driver, distance we have covered so far is : " << distance_covered << endl;
		// Wake up the other threads that are sleeping on condition variable cv.
		if (distance_covered == total_distance) { cv.notify_one(); };

		if (distance_covered == 15) { break; }
	}
}
void ask_driver_to_wake_u_up_at_right_time()
{
	// We can not use lock_guard<mutex> because it locks the mutex on the constructor and doesn't have lock and unlock calls.
	// Where as unique_lock doesn't lock the wrapped mutex on construction and has lock unlock capability. Lock unlock is
	// required by condition variable. Remember cv wait will first call ul.lock() to lock the mutex, then check the condition and will call ul.unlock()
	// to unlock the mutex when it sees that the condition doesn't hold, and make thread go back to sleep until some other threads wakes it up on that condition variable again.
	// If the condition holds, then cv.wait will finish and when going out of scope unique_lock will be destructed which will unlock the mutex on
	// destructor. Also remember that OS can wake the thread sleeping on cv spuriously even thought the condition hasn't yet been
	// satisfied. If so cv.wait() will lock ul, check that the condition doesn't hold and unlock ul and go back to sleep.
	unique_lock<mutex> ul(m2);
	// cv.wait will lock the ul on cv.wait at first. In the first moment condition will not be true since distance covered is yet 0
	// Then cv will make this thread sleep. It will wake up when another thread calls cv.notify_one();
	cv.wait(ul, [] {return distance_covered >= total_distance; });
	cout << "Finally I am there, distance_covered = " << distance_covered << endl;
}
void Examples::condition_variable_run()
{
	cout << "\n\nCONDITION VARIABLE_________________________________" << endl;
	thread driver_thread{ keep_moving };
	thread passenger_thread{ ask_driver_to_wake_u_up_at_right_time };

	passenger_thread.join();
	driver_thread.join();

}

int find_answer_how_old_universe_is()
{
	// assume this is a function which will take a long time to compute, therefore it will be called async so that 
	// main thread doesn't have to wait for it
	this_thread::sleep_for(chrono::milliseconds(1000));
	return 5000;
}
void do_other_calculations()
{
	cout << "Main thread is continuing its calculations without waiting for future thread" << endl;
	this_thread::sleep_for(chrono::milliseconds(200));
	cout << "Main threa calculations are done" << endl;
}

void Examples::future_run()
{
	cout << "\n\nASYNC OPERATIONS AND FUTURE OBJECT_________________________________" << endl;

	// This is not blocking main thread. find_answer_how_old_universe_is() will be executed in another thread.
	future<int> the_answer_future = async(find_answer_how_old_universe_is);  
	do_other_calculations();  // main thread directly jumps to continue doing its own stuff
	cout << "Main thread needs the answer from future at this point, if future hasn't yet finished, main thread will block" << endl;
	cout << "Calling future.get() to get the result from the future thread" << endl;
	cout << "The answer is " << the_answer_future.get() << endl;
}


int add_(int x, int y)
{
	cout << "Addition runs on thread: " << this_thread::get_id() << endl;
	return x + y;
}
int subtract(int x, int y)
{
	cout << "Subtraction runs on thread: " << this_thread::get_id() << endl;
	return x - y;
}
void hello(const string & name)
{
	cout << "hello runs on thread: " << this_thread::get_id() << endl;
}
void Examples::future_run_2()
{
	cout << "\n\nASYNC OPERATIONS AND FUTURE OBJECT (ASYNC OR DEFERRED)_________________________________" << endl;
	future<int> future1 = async(std::launch::async, add_, 10, 15); // Function runs on separate thread in parallel
	future<int> future2 = async(std::launch::deferred, subtract, 9, 4);  // Function runs on this thread (deferred) as soon as future.get() is called.
	future<void> future3 = async(std::launch::async | std::launch::deferred, hello, "Zahit"); // Compiler decides whether function will run on separate thread or not

	cout << "Main thread id: " << this_thread::get_id() << endl;
	cout << "Value received using future1 : " << future1.get() << endl;
	cout << "Value received using future2 : " << future2.get() << endl;
	future3.get();
}

void Examples::packaged_task_run()
{
	cout << "\n\nPACKAGED TASK_________________________________" << endl;
	packaged_task<int(int, int)> task1(add_);
	future<int> future1 = task1.get_future();  // Note that future template param is int because add returns int
	cout << "Main thread id: " << this_thread::get_id() << endl;
	task1(5, 15); // Unlike async, you have to call the task explicitly
	cout << "Task normal result: " << future1.get() << endl;

	packaged_task<int(int, int)> task2(add_);
	future<int> future2 = task2.get_future();
	thread t{ std::move(task2), 17, 8 };  // If you want the packaged task to be executed on a separate thread create a thread, but you have to use std::move to construct thread
	t.detach();
	cout << "Task thread result: " << future2.get() << endl;

}

void print_int(future<int>& fut)
{
	cout << "Waiting for value in print_thread" << endl;
	cout << "value is ready as promised: " << fut.get() << endl;
}
void Examples::promise_run()
{
	cout << "\n\nPROMISE_________________________________" << endl;
	// You create a promise and get future object from it. This promise will be used to pass value from main thread to child thread.
	// Main thread uses the promise object to set_value() and child thread reads the value using fut object passed to it with std::ref(fut)
	promise<int> prom;
	future<int> fut = prom.get_future();

	thread print_thread(print_int, std::ref(fut));

	this_thread::sleep_for(chrono::milliseconds(1000));
	cout << "Setting the value in main thread" << endl;
	prom.set_value(100);

	print_thread.join();
}

void print_func(future<int> & fut)
{
	cout << "Inside print_func waiting for future.get()" << endl;

	try {
		cout << "Got the value: " << fut.get() << endl;  // If exception is set from prom.set_exception() it will be caught in this thread as well
	}
	catch (std::exception & e) {
		cout << "Exception caught in print_thread: " << this_thread::get_id() << " : " << e.what() << endl;
	}
}
void calculate_sqrt(promise<int> & prom)
{
	int x = 1;
	cout << "Please, enter an integer value: ";
	
	try {
		cin >> x;
		if (x < 0) throw exception("ERROR: Negative value");
		prom.set_value(sqrt(x));
	}
	catch (std::exception & e) {
		cout << "Exception caught in calculate_sqrt_thread: " << this_thread::get_id() << " : " << e.what() << endl;
		prom.set_exception(current_exception()); // Propagating the exception to thread which is going to call fut.get()
	}


}
void Examples::promise_exception_propagate_run()
{
	cout << "\n\nPROMISE EXCEPTION PROPAGATION_________________________________" << endl;
	promise<int> prom;
	future<int> fut = prom.get_future();

	thread print_thread{ print_func, std::ref(fut) };
	thread calculate_sqrt_thread{ calculate_sqrt, std::ref(prom) };

	print_thread.join();
	calculate_sqrt_thread.join();
}

void print_func2(shared_future<int>& fut)
{
	// if future<int> & was used instead of shared_future<int>, then both threads that are passed the same future will call fut.get()
	// The first fut.get() will succeed but the second will make the program crash. You can still use fut.valid() to check if fut.get()
	// is already called or not. But even still you could run into race condition meaning both threads call fut.valid() one after the
	// other and both see that the fut.get() is not called yet. Then they both try fut.get() resulting in the same problem.
	cout << fut.get() << " - valid future\n" << endl;
}

void Examples::shared_future_run()
{
	cout << "\n\nSHARED FUTURE_________________________________" << endl;
	promise<int> prom;
	shared_future<int> fut(prom.get_future());

	thread t1{ print_func2, std::ref(fut) };
	thread t2{ print_func2, std::ref(fut) };


	prom.set_value(88);

	t1.join();
	t2.join();
}
