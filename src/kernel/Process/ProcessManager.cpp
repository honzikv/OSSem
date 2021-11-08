#include "ProcessManager.h"
#include "kernel.h"
#include "handles.h"

kiv_os::THandle ProcessManager::GetFreePid() const {
	for (uint16_t i = 0; i < static_cast<uint16_t>(process_table.size()); i += 1) {
		if (process_table[i] == nullptr) {
			return i + PID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}

kiv_os::THandle ProcessManager::GetFreeTid() const {
	for (uint16_t i = 0; i < static_cast<uint16_t>(thread_table.size()); i += 1) {
		if (thread_table[i] == nullptr) {
			return i + TID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}

std::shared_ptr<Process> ProcessManager::GetProcess(const kiv_os::THandle pid) {
	return process_table[pid - PID_RANGE_START];
}

void ProcessManager::AddProcess(const std::shared_ptr<Process> process, const kiv_os::THandle pid) {
	process_table[pid] = process;
}

std::shared_ptr<Thread> ProcessManager::GetThread(const kiv_os::THandle tid) {
	return thread_table[tid - TID_RANGE_START];
}

auto ProcessManager::AddThread(const std::shared_ptr<Thread> thread, const kiv_os::THandle tid) -> void {
	thread_table[tid - TID_RANGE_START] = thread;
}

kiv_os::NOS_Error ProcessManager::ProcessSyscall(kiv_hal::TRegisters& regs) {
	// Spustime jednotlive funkce podle operace
	auto operation = regs.rax.l;
	switch (static_cast<kiv_os::NOS_Process>(operation)) {
		case kiv_os::NOS_Process::Clone:
			return PerformClone(regs);

		case kiv_os::NOS_Process::Wait_For:
			return PerformWaitFor(regs);

		case kiv_os::NOS_Process::Read_Exit_Code:
			return PerformReadExitCode(regs);

		case kiv_os::NOS_Process::Exit:
			return PerformProcessExit(regs);

		case kiv_os::NOS_Process::Shutdown:
			return PerformShutdown(regs);

		case kiv_os::NOS_Process::Register_Signal_Handler:
			return PerformRegisterSignalHandler(regs);
	}
}

kiv_os::NOS_Error ProcessManager::PerformClone(kiv_hal::TRegisters& regs) {
	const auto operationType = static_cast<kiv_os::NClone>(regs.rcx.l);
	if (operationType == kiv_os::NClone::Create_Process) {
		return CreateProcess(regs);
	}
	if (operationType == kiv_os::NClone::Create_Thread) {
		return CreateThread(regs);
	}

	// Jinak vratime invalid argument
	return kiv_os::NOS_Error::Invalid_Argument;
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::FindParentPid() {
	const auto tid_handle = std::this_thread::get_id();
	const auto lock = std::scoped_lock(tasks_mutex);
	// Pokud nelze tid prevest vratime invalid handle
	if (native_tid_to_kiv_handle.count(tid_handle) == 0) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak ziskame vlakno z tabulky a pid jeho procesu
	const auto tid = native_tid_to_kiv_handle[tid_handle];
	return thread_table[tid - TID_RANGE_START]->GetPid();
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::GetCurrentThreadTid() {
	const auto lock = std::scoped_lock(tasks_mutex);
	return native_tid_to_kiv_handle[std::this_thread::get_id()];
}


kiv_os::NOS_Error ProcessManager::CreateProcess(kiv_hal::TRegisters& regs) {
	// Ziskame jmeno programu a argumenty
	const auto programName = std::string(reinterpret_cast<char*>(regs.rdx.r)); // NOLINT(performance-no-int-to-ptr)
	const auto programArgs = std::string(reinterpret_cast<char*>(regs.rdi.r)); // NOLINT(performance-no-int-to-ptr)

	// Ziskame funkci s programem a pretypujeme ji na TThread_Proc
	const auto program = reinterpret_cast<kiv_os::TThread_Proc>(GetProcAddress(User_Programs, programName.c_str()));
	// Pokud program neexistuje vratime Invalid_Argument
	if (!program) {
		regs.flags.carry = 1;
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	// bx.e = (stdin << 16) | stdout
	const auto std_out = static_cast<uint16_t>(regs.rbx.e & 0xffff);
	// vymaskujeme prvnich 16 msb a pretypujeme na uint16
	const auto std_in = static_cast<uint16_t>(regs.rbx.e >> 16 & 0xffff);
	// posuneme o 16 bitu a vymaskujeme prvnich 16 lsb

	// Vytvorime zamek, protoze ziskame pid a tid, ktere nam nesmi nikdo sebrat
	auto lock = std::scoped_lock(tasks_mutex);
	// Nejprve provedeme check zda-li muzeme vytvorit novy proces - musi existovat volny pid a tid
	const auto pid = GetFreePid();
	const auto tid = GetFreeTid();

	if (pid == NO_FREE_ID || tid == NO_FREE_ID) {
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Nastavime registry pro proces a predame je hlavnimu vlaknu
	auto process_context = kiv_hal::TRegisters();
	process_context.rax.x = std_in;
	process_context.rbx.x = std_out;

	// Vytvorime vlakno procesu, kde se spusti program
	const auto main_thread = std::make_shared<Thread>(program, process_context, tid, pid);

	// Zjistime, zda-li vlakno, ve kterem se proces vytvari ma nejakeho rodice a nastavime ho (pokud existuje
	// jinak se nastavi invalid value)
	const auto parent_process_pid = FindParentPid();
	const auto process = std::make_shared<Process>(pid, tid, parent_process_pid, std_in, std_out);

	// Pridame proces a vlakno do tabulky
	AddProcess(process, pid);
	AddThread(main_thread, tid);

	// Spustime vlakno
	const auto thread_handle = main_thread->Dispatch();
	native_tid_to_kiv_handle[thread_handle] = tid;
	kiv_handle_to_native_tid[tid] = thread_handle;
	process->SetRunning();
	regs.rax.x = pid;

	// Vratime success
	return kiv_os::NOS_Error::Success;
}

void ProcessManager::TriggerSuspendCallback(const kiv_os::THandle subscriber_handle,
                                              const kiv_os::THandle notifier_handle) {
	auto lock = std::scoped_lock(suspend_callbacks_mutex);
	const auto callback = suspend_callbacks[subscriber_handle];
	if (callback == nullptr) {
		LogWarning("Callback was nullptr but notify was attempted by pid/tid: " + notifier_handle);
		return;
	}

	callback->Notify(notifier_handle);
}

void ProcessManager::FinishProcess(const kiv_os::THandle pid) {
	LogDebug("Terminating: " + pid);
	std::shared_ptr<Process> process;
	{
		const auto lock = std::scoped_lock(tasks_mutex);
		process = GetProcess(pid);
		for (const auto tid : process->GetProcessThreads()) {
			const auto thread = GetThread(tid);
			if (thread == nullptr) {
				// tento stav by nicmene snad nikdy nemel nastav
				continue;
			}

			// Ukoncime vsechna vlakna (pokud se neukoncili)
			thread->Finish(thread->GetTid());

			// Smazeme vlakno a zaznamy z lookup tabulek
			thread_table[tid - TID_RANGE_START] = nullptr;
			const auto native_tid = kiv_handle_to_native_tid[tid];
			kiv_handle_to_native_tid.erase(tid);
			native_tid_to_kiv_handle.erase(native_tid);
		}

		// Smazeme zaznam o procesu
		process_table[pid - PID_RANGE_START] = nullptr;
	}

	// Zavolame vsechny vlakna / procesy cekajici na ukonceni procesu
	process->Finish(process->GetPid());
}

HandleType ProcessManager::GetHandleType(const kiv_os::THandle id) {
	if (id >= PID_RANGE_START && id < PID_RANGE_END) {
		// Pokud je handle mezi 0 - PID_RANGE_END jedna se o proces
		return HandleType::PROCESS;
	}
	// Jinak musi byt handle mezi TID_RANGE_START a TID_RANGE_END
	return id >= TID_RANGE_START && id < TID_RANGE_END ? HandleType::THREAD : HandleType::INVALID;
}

void ProcessManager::AddCurrentThreadAsSubscriber(const kiv_os::THandle* handle_array,
                                                      const uint32_t handle_array_size,
                                                      const kiv_os::THandle current_tid) {
	// Pro validaci musime locknout tabulku
	// Validujeme protoze se muze stat, ze procesy mezitim co jsme provadeli metodu
	// dobehli, a byly odstranene z tabulek
	auto lock = std::scoped_lock(tasks_mutex);
	for (uint32_t i = 0; i < handle_array_size; i += 1) {
		const auto handle = handle_array[i];
		const auto handleType = GetHandleType(handle);

		if (handleType == HandleType::PROCESS) {
			// Pokud se jedna o proces, tak do nej pridame subscribera do nej
			// Proces po dobehnuti vsechny subscribery probudi
			const auto process = GetProcess(handle);
			if (process == nullptr) { continue; }
			process->AddSubscriber(current_tid);
			continue;
		}

		if (handleType == HandleType::THREAD) {
			// Pokud se jedna o vlakno, udelame to same, ale staci aby dobehlo vlakno
			const auto thread = GetThread(handle);
			if (thread == nullptr) { continue; }
			thread->AddSubscriber(current_tid);
		}
		// Pokud je hodnota HandleType::Invalid, nic nedelame
	}
}

kiv_os::NOS_Error ProcessManager::PerformWaitFor(kiv_hal::TRegisters& regs) {
	const auto handle_array = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r);
	const auto handle_count = regs.rcx.e;

	// Thread id aktualne beziciho vlakna
	const auto current_tid = GetCurrentThreadTid();

	// Pridame vlakno do vsech existujicich procesu/vlaken v handle_array
	AddCurrentThreadAsSubscriber(handle_array, handle_count, current_tid);

	// Nastavime vychozi hodnotu na invalid value, pokud se na neco opravdu cekalo prenastavime
	regs.rax.x = static_cast<decltype(regs.rax.x)>(kiv_os::Invalid_Handle);
	std::shared_ptr<SuspendCallback> callback; // nullptr

	// Musime locknout aby nedoslo k race condition
	{
		auto lock = std::scoped_lock(suspend_callbacks_mutex);
		callback = suspend_callbacks[current_tid]; // ziskame callback
	}

	// Muze se stat, ze vsechny prvky z pole uz se ukoncili pred tim, nez se inicializovalo wait for
	// (pripadne, ze se zadali spatne hodnoty)
	if (callback == nullptr) {
		return kiv_os::NOS_Error::Success;
	}

	// Pokud se callback jeste nespustil uspime vlakno
	if (!callback->Triggered()) {
		callback->Suspend();
	}

	// Ziskame id vlakna/procesu, ktery toto vlakno vzbudil
	regs.rax.x = callback->Get_Notifier_Id();

	// Smazeme zaznam z mapy callbacku
	RemoveSuspendCallback(current_tid);

	return kiv_os::NOS_Error::Success;
}

void ProcessManager::CreateInitProcess() {
	const auto pid = GetFreePid();
	const auto tid = GetFreeTid();

	// "Funkce", ktera se ma spustit
	auto process = std::make_shared<Process>(pid, tid, kiv_os::Invalid_Handle, kiv_os::Invalid_Handle,
	                                         kiv_os::Invalid_Handle);

	auto func = [](const kiv_hal::TRegisters& _) -> size_t { return 0; };
	auto thread = std::make_shared<Thread>(func, kiv_hal::TRegisters(), tid, pid);

	// Pridame do tabulky
	AddProcess(process, pid);
	AddThread(thread, tid);
	const auto native_tid = std::this_thread::get_id();
	native_tid_to_kiv_handle[native_tid] = tid;
	kiv_handle_to_native_tid[tid] = native_tid;
}

void ProcessManager::InitializeSuspendCallback(const kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(suspend_callbacks_mutex);
	if (suspend_callbacks[subscriber_handle] == nullptr) {
		suspend_callbacks[subscriber_handle] = std::make_shared<SuspendCallback>();
	}
}

void ProcessManager::RemoveSuspendCallback(kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(suspend_callbacks_mutex);
	suspend_callbacks[subscriber_handle] = nullptr;
}
