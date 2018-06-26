#pragma once
#include <thread>

class thread_guard
{
	std::thread& t;
public:


	explicit thread_guard(std::thread& t)
		: t(t)
	{
	}

	// So here in destructor we make sure that the thread that is wrapped inside this class gets joined
	// during stack unwind. Here RAII principle is used. Since we know that all local objects on stack will be
	// destructed on stack unwind (whether due to an exception or simply the local scope being exited), then this
	// makes sure that the created thread is joined.
	~thread_guard()
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	thread_guard(const thread_guard &) = delete;
	thread_guard& operator=(const thread_guard&) = delete;
};
