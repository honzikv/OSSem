#include "Thread.h"

#include <iostream>

#include "Utils/Logging.h"

void Thread::Thread_Func() {
	program(regs); // Spusteni programu
}

std::thread::id Thread::Init() {
	Log("Thread init");
	auto thread = std::thread(&Thread::Thread_Func, this);
	const auto thread_id = thread.get_id();
	thread.detach(); // vlakno musime detachnout
	return thread_id; // vratime thread id
}
