#include "Process.h"
#include "ProcessManager.h"

Process::Process(const kiv_os::THandle pid, const kiv_os::THandle main_thread_tid, const kiv_os::THandle parent_pid,
                 const kiv_os::THandle std_in, const kiv_os::THandle std_out): pid(pid), parent_pid(parent_pid),
                                                                               std_in(std_in), std_out(std_out) {
	threads.push_back(main_thread_tid);
}

void Process::AddThread(const kiv_os::THandle tid) {
	threads.push_back(tid);
}
