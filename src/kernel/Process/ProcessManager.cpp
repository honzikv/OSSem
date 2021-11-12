#include "ProcessManager.h"
#include "../kernel.h"
#include "../IO/IOManager.h"

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
	if (pid >= PID_RANGE_START && pid < PID_RANGE_END) {
		return process_table[pid - PID_RANGE_START];
	}
	return nullptr;
}

void ProcessManager::AddProcess(const std::shared_ptr<Process> process, const kiv_os::THandle pid) {
	process_table[pid] = process;
}

std::shared_ptr<Thread> ProcessManager::GetThread(const kiv_os::THandle tid) {
	if (tid >= TID_RANGE_START && tid < TID_RANGE_END) {
		return thread_table[tid - TID_RANGE_START];
	}
	return nullptr;
}

auto ProcessManager::AddThread(const std::shared_ptr<Thread> thread, const kiv_os::THandle tid) -> void {
	thread_table[tid - TID_RANGE_START] = thread;
}

void ProcessManager::ProcessSyscall(kiv_hal::TRegisters& regs) {
	// Spustime jednotlive funkce podle operace
	auto operation = regs.rax.l;
	auto operation_result = kiv_os::NOS_Error::Success;
	switch (static_cast<kiv_os::NOS_Process>(operation)) {
		case kiv_os::NOS_Process::Clone: {
			operation_result = PerformClone(regs);
			break;
		}

		case kiv_os::NOS_Process::Wait_For: {
			operation_result = PerformWaitFor(regs);
			break;
		}

		case kiv_os::NOS_Process::Read_Exit_Code: {
			operation_result = PerformReadExitCode(regs, false);
			break;
		}

		case kiv_os::NOS_Process::Exit: {
			operation_result = PerformReadExitCode(regs, true);
			break;
		}

		case kiv_os::NOS_Process::Shutdown: {
			operation_result = PerformShutdown(regs);
			break;
		}

		case kiv_os::NOS_Process::Register_Signal_Handler: {
			operation_result = PerformRegisterSignalHandler(regs);
			break;
		}
		default: {
			break;
		}
	}

	if (operation_result != kiv_os::NOS_Error::Success) {
		regs.flags.carry = 1;
		regs.rax.x = static_cast<decltype(regs.rax.x)>(operation_result);
	}
}

kiv_os::NOS_Error ProcessManager::PerformClone(kiv_hal::TRegisters& regs) {
	const auto operationType = static_cast<kiv_os::NClone>(regs.rcx.l);
	if (operationType == kiv_os::NClone::Create_Process) {
		return CreateNewProcess(regs);
	}
	if (operationType == kiv_os::NClone::Create_Thread) {
		return CreateNewThread(regs);
	}

	// Jinak vratime invalid argument
	return kiv_os::NOS_Error::Invalid_Argument;
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::FindParentPid() {
	const auto current_thread_handle = GetCurrentThreadId();
	const auto lock = std::scoped_lock(tasks_mutex);
	// Pokud nelze tid prevest vratime invalid handle
	if (thread_id_to_kiv_handle.count(current_thread_handle) == 0) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak ziskame vlakno z tabulky a pid jeho procesu
	const auto tid = thread_id_to_kiv_handle[current_thread_handle];
	return thread_table[tid - TID_RANGE_START]->GetPid();
}

// ReSharper disable once CppMemberFunctionMayBeConst
kiv_os::THandle ProcessManager::GetCurrentTid() {
	const auto current_thread = GetCurrentThreadId();
	const auto lock = std::scoped_lock(tasks_mutex);
	return thread_id_to_kiv_handle[current_thread];
}

HANDLE ProcessManager::GetThreadNativeHandle(const kiv_os::THandle tid) {
	auto lock = std::scoped_lock(tasks_mutex);
	const auto native_thread_id = kiv_handle_to_thread_id[tid];
	return native_thread_id_to_native_handle[native_thread_id];
}


kiv_os::NOS_Error ProcessManager::CreateNewProcess(kiv_hal::TRegisters& regs) {
	// Ziskame jmeno programu a argumenty
	const auto program_name = reinterpret_cast<char*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto program_args = reinterpret_cast<char*>(regs.rdi.r); // NOLINT(performance-no-int-to-ptr)

	// Ziskame funkci s programem a pretypujeme ji na TThread_Proc
	const auto program = reinterpret_cast<kiv_os::TThread_Proc>(GetProcAddress(User_Programs, program_name));
	// Pokud program neexistuje vratime Invalid_Argument
	if (!program) {
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	// bx.e = (stdin << 16) | stdout
	auto std_in = static_cast<uint16_t>(regs.rbx.e >> 16 & 0xffff);
	// posuneme o 16 bitu a vymaskujeme prvnich 16 lsb
	auto std_out = static_cast<uint16_t>(regs.rbx.e & 0xffff);
	// vymaskujeme prvnich 16 msb a pretypujeme na uint16

	const auto [process_std_in, process_std_out] = IOManager::Get().MapProcessStdio(std_in, std_out);

	LogDebug("Creating new process for_command: " + std::string(program_name)
		+ "\nSetting file descriptors stdin=" + std::to_string(process_std_in) + " stdout=" + std::to_string(
			process_std_out));

	// Vytvorime zamek, protoze ziskame pid a tid, ktere nam nesmi nikdo sebrat
	auto lock = std::scoped_lock(tasks_mutex);
	// Nejprve provedeme check zda-li muzeme vytvorit novy proces - musi existovat volny pid a tid
	const auto pid = GetFreePid();
	const auto tid = GetFreeTid();

	if (pid == NO_FREE_ID || tid == NO_FREE_ID) {
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Nastavime stdin a stdout pro proces a predame je hlavnimu vlaknu
	// Argumenty programu se kopiruji do objektu Thread a ten si je nastavi sam
	auto process_context = kiv_hal::TRegisters();
	process_context.rax.x = process_std_in;
	process_context.rbx.x = process_std_out;

	// Vytvorime vlakno procesu, kde se spusti program
	const auto main_thread = std::make_shared<Thread>(program, process_context, tid, pid, program_args);

	// Zjistime, zda-li vlakno, ve kterem se proces vytvari ma nejakeho rodice a nastavime ho (pokud existuje
	// jinak se nastavi invalid value)
	const auto parent_process_pid = FindParentPid();
	const auto process = std::make_shared<Process>(pid, tid, parent_process_pid, std_in, std_out);

	// Pridame proces a vlakno do tabulky
	AddProcess(process, pid);
	AddThread(main_thread, tid);

	// Spustime vlakno
	const auto [native_handle, native_id] = main_thread->Dispatch();

	// Namapujeme nativni tid na THandle
	thread_id_to_kiv_handle[native_id] = tid;
	kiv_handle_to_thread_id[tid] = native_id;
	native_thread_id_to_native_handle[native_id] = native_handle;
	process->SetRunning();
	regs.rax.x = pid;

	// Vratime success
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::CreateNewThread(kiv_hal::TRegisters& regs) {
	// Ziskame jmeno programu a argumenty
	const auto program_name = reinterpret_cast<char*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto program_args = reinterpret_cast<char*>(regs.rdi.r); // NOLINT(performance-no-int-to-ptr)

	// Ziskame funkci s programem a pretypujeme ji na TThread_Proc
	const auto program = reinterpret_cast<kiv_os::TThread_Proc>(GetProcAddress(User_Programs, program_name));
	// Pokud program neexistuje vratime Invalid_Argument
	if (!program) {
		return kiv_os::NOS_Error::Invalid_Argument;
	}

	// bx.e = (stdin << 16) | stdout
	const auto std_out = static_cast<uint16_t>(regs.rbx.e & 0xffff);
	// vymaskujeme prvnich 16 msb a pretypujeme na uint16
	const auto std_in = static_cast<uint16_t>(regs.rbx.e >> 16 & 0xffff);
	// posuneme o 16 bitu a vymaskujeme prvnich 16 lsb

	// Vytvorime zamek, protoze ziskame tid, ktery by nikdo nemel sebrat
	auto lock = std::scoped_lock(tasks_mutex);
	auto tid = GetFreeTid();
	if (tid == NO_FREE_ID) {
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Nastavime stdin a stdout pro proces a predame je hlavnimu vlaknu
	// Argumenty programu se kopiruji do objektu Thread a ten si je nastavi sam
	auto thread_context = kiv_hal::TRegisters();
	thread_context.rax.x = std_in;
	thread_context.rbx.x = std_out;

	// Ziskame aktualni proces
	const auto current_thread = GetThread(GetCurrentTid());
	if (current_thread == nullptr) {
		// toto by nastat nicmene nemelo
		return kiv_os::NOS_Error::File_Not_Found;
	}

	const auto current_process = GetProcess(current_thread->GetPid());
	if (current_process == nullptr) {
		// toto by nastat nicmene nemelo
		return kiv_os::NOS_Error::File_Not_Found;
	}
	

	const auto thread = std::make_shared<Thread>(program, thread_context, tid, current_process->GetPid(),
	                                             program_args, false);

	// Pridame vlakno do tabulky a spustime
	LogDebug("New thread created with tid: " + std::to_string(tid));
	AddThread(thread, tid);
	current_process->AddThread(tid); // Pridame vlakno k procesu
	const auto [native_handle, native_tid] = thread->Dispatch();

	// Namapujeme nativni tid na THandle
	thread_id_to_kiv_handle[native_tid] = tid;
	kiv_handle_to_thread_id[tid] = native_tid;
	native_thread_id_to_native_handle[native_tid] = native_handle;
	regs.rax.x = tid; // Vratime zpet tid vlakna

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

void ProcessManager::TerminateProcess(const kiv_os::THandle pid) {
	std::shared_ptr<Process> process;
	{
		const auto lock = std::scoped_lock(tasks_mutex);
		process = GetProcess(pid);

		if (process == nullptr) {
			return;
		}

		for (const auto tid : process->GetProcessThreads()) {
			const auto thread = GetThread(tid);
			if (thread == nullptr) {
				// tento stav by nicmene snad nikdy nemel nastav
				continue;
			}

			// Notifikujeme cekajici tasky na toto vlakno
			thread->SignalSubscribers(thread->GetTid(), false);

			// Pokud vlakno stale bezi ukoncime ho
			thread->Terminate(GetThreadNativeHandle(thread->GetTid()));
		}
	}

	// Zavolame vsechny vlakna / procesy cekajici na ukonceni procesu
	process->SignalSubscribers(process->GetPid());

	IOManager::Get().CloseProcessStdio(process->GetStdIn(), process->GetStdOut());
}

HandleType ProcessManager::GetHandleType(const kiv_os::THandle id) {
	if (id >= PID_RANGE_START && id < PID_RANGE_END) {
		// Pokud je handle mezi 0 - PID_RANGE_END jedna se o proces
		return HandleType::Process;
	}
	// Jinak musi byt handle mezi TID_RANGE_START a TID_RANGE_END
	return id >= TID_RANGE_START && id < TID_RANGE_END ? HandleType::Thread : HandleType::INVALID;
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

		if (handleType == HandleType::Process) {
			// Pokud se jedna o proces, tak do nej pridame subscribera do nej
			// Proces po dobehnuti vsechny subscribery probudi
			const auto process = GetProcess(handle);
			if (process == nullptr) { continue; }
			process->AddSubscriber(current_tid);
			continue;
		}

		if (handleType == HandleType::Thread) {
			// Pokud se jedna o vlakno, udelame to same, ale staci aby dobehlo vlakno
			const auto thread = GetThread(handle);
			if (thread == nullptr) { continue; }
			thread->AddSubscriber(current_tid);
		}
		// Pokud je hodnota HandleType::Invalid, nic nedelame
	}
}

kiv_os::NOS_Error ProcessManager::PerformWaitFor(kiv_hal::TRegisters& regs) {
	const auto handle_array = reinterpret_cast<kiv_os::THandle*>(regs.rdx.r); // NOLINT(performance-no-int-to-ptr)
	const auto handle_count = regs.rcx.e;
	
	// Thread id aktualne beziciho vlakna
	const auto current_tid = GetCurrentTid();

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

void ProcessManager::RemoveProcess(const std::shared_ptr<Process> process) {
	{
		auto lock = std::scoped_lock(tasks_mutex);
		// Odstranime vlakna procesu
		for (const auto& tid : process->GetProcessThreads()) {
			const auto thread = GetThread(tid);
			if (thread != nullptr) {
				RemoveThread(thread);
			}
		}
		process_table[process->GetPid() - PID_RANGE_START] = nullptr; // odstranime proces z tabulky
	}

	// zavreme stdin a stdout
}

void ProcessManager::RemoveThread(std::shared_ptr<Thread> thread) {
	const auto tid = thread->GetTid();
	auto lock = std::scoped_lock(tasks_mutex);
	// Odstranime vlakno
	thread_table[tid - TID_RANGE_START] = nullptr;
	const auto native_tid = kiv_handle_to_thread_id[tid];
	kiv_handle_to_thread_id.erase(tid);
	thread_id_to_kiv_handle.erase(native_tid);
	native_thread_id_to_native_handle.erase(native_tid);
}

kiv_os::NOS_Error ProcessManager::PerformReadExitCode(kiv_hal::TRegisters& regs, bool remove_task) {
	// Id handlu, ktery se ma precist
	const auto handle = regs.rdx.x;

	// Zjistime, zda-li se jedna o vlakno nebo proces
	const auto handle_type = GetHandleType(handle);

	// Ziskame vlakno / proces, pro ktery exit code cteme a odstranime jeho zaznamy z tabulky
	std::shared_ptr<Task> task = nullptr;
	if (handle_type == HandleType::Process) {
		task = GetProcess(handle);
		if (remove_task) {
			RemoveProcess(std::static_pointer_cast<Process>(task)); // pretypovani Task na Process shared ptr
		}
	}
	else if (handle_type == HandleType::Thread) {
		task = GetThread(handle);
		if (remove_task) {
			RemoveThread(std::static_pointer_cast<Thread>(task)); // pretypovani Task na Thread shared ptr
		}
	}

	// V pripade, ze se task nepodarilo najit, nebo nebyl proces / vlakno
	if (task == nullptr) {
		regs.flags.carry = 1;
		regs.rcx.r = static_cast<uint16_t>(kiv_os::NOS_Error::File_Not_Found);
		return kiv_os::NOS_Error::File_Not_Found;
	}

	// Zjistime exit code a vratime se
	const auto exit_code = task->GetExitCode();
	regs.rcx.x = static_cast<decltype(regs.rcx.x)>(exit_code);
	return kiv_os::NOS_Error::Success;
}

kiv_os::NOS_Error ProcessManager::PerformShutdown(const kiv_hal::TRegisters& regs) {
	LogDebug("Shutdown performed");
	auto lock = std::scoped_lock(tasks_mutex);

	// Init proces se ukonci sam a vzdy ma pid 1
	for (auto pid = 1; pid < PID_RANGE_END; pid += 1) {
		auto process = process_table[pid];
		if (process == nullptr) {
			continue;
		}

		TerminateProcess(pid);
		RemoveProcess(std::static_pointer_cast<Process>(process));
	}

	return kiv_os::NOS_Error::Success;
}

void ProcessManager::RunInitProcess(kiv_os::TThread_Proc program) {
	LogDebug("Creating init process");
	const auto pid = PID_RANGE_START;
	const auto tid = GetFreeTid();

	const auto process = std::make_shared<Process>(pid, tid, kiv_os::Invalid_Handle, kiv_os::Invalid_Handle,
	                                               kiv_os::Invalid_Handle);
	// "Funkce", ktera se ma spustit
	const auto args = "";
	const auto thread = std::make_shared<Thread>(program, kiv_hal::TRegisters(), tid, pid, args);

	auto lock = std::scoped_lock(tasks_mutex);
	auto [native_handle, native_tid] = thread->Dispatch();
	// Pridame do tabulky
	AddProcess(process, pid);
	AddThread(thread, tid);
	thread_id_to_kiv_handle[native_tid] = tid;
	kiv_handle_to_thread_id[tid] = native_tid;
	native_thread_id_to_native_handle[native_tid] = native_handle;
}

void ProcessManager::InitializeSuspendCallback(const kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(suspend_callbacks_mutex);
	if (suspend_callbacks.count(subscriber_handle) == 0) {
		suspend_callbacks[subscriber_handle] = std::make_shared<SuspendCallback>();
	}
}

void ProcessManager::RemoveSuspendCallback(const kiv_os::THandle subscriber_handle) {
	auto lock = std::scoped_lock(suspend_callbacks_mutex);
	suspend_callbacks[subscriber_handle] = nullptr;
}
