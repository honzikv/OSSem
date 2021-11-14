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

kiv_os::THandle Process::GetPid() const { return pid; }

kiv_os::THandle Process::GetParentPid() const { return parent_pid; }

kiv_os::THandle Process::GetStdIn() const { return std_in; }

kiv_os::THandle Process::GetStdOut() const { return std_out; }

std::string& Process::GetWorkingDir() { return working_dir; }

void Process::SetWorkingDir(const std::string dir) {
	working_dir = dir;
}

void Process::SetSignalCallback(const kiv_os::NSignal_Id signal, const kiv_os::TThread_Proc callback) {
	signal_callbacks[signal] = callback;
}
