#include "Process.h"
#include "ProcessManager.h"


Process::Process(const kiv_os::THandle pid, const kiv_os::THandle main_thread_tid, const kiv_os::THandle parent_pid,
                 const kiv_os::THandle std_in, const kiv_os::THandle std_out, Path& working_dir):
	pid(pid), parent_pid(parent_pid), std_in(std_in),
	std_out(std_out), working_dir(std::move(working_dir)) {

	threads.push_back(main_thread_tid);
}

void Process::AddThread(const kiv_os::THandle tid) {
	threads.push_back(tid);
}

kiv_os::THandle Process::Get_Pid() const { return pid; }

kiv_os::THandle Process::GetParentPid() const { return parent_pid; }

kiv_os::THandle Process::GetStdIn() const { return std_in; }

kiv_os::THandle Process::GetStdOut() const { return std_out; }

Path& Process::GetWorkingDir() { return working_dir; }

void Process::SetWorkingDir(Path& path) {
	auto lock = std::scoped_lock(subscribers_mutex);
	working_dir = std::move(path);
}


void Process::SetSignalCallback(const kiv_os::NSignal_Id signal, const kiv_os::TThread_Proc callback) {
	signal_callbacks[signal] = callback;
}

bool Process::HasCallbackForSignal(int signal_number) const {
	return signal_callbacks.count(static_cast<kiv_os::NSignal_Id>(signal_number)) != 0;
}

// ReSharper disable once CppMemberFunctionMayBeConst
void Process::Execute_Signal_Callback(kiv_os::NSignal_Id signal_id) {
	Log_Debug("Executing signal callback");
	auto regs = kiv_hal::TRegisters();
	regs.rcx.r = static_cast<decltype(regs.rcx.r)>(signal_id);
	signal_callbacks[signal_id](regs);
}
