#include "ProcessManager.h"
#include "kernel.h"
#include "handles.h"

kiv_os::THandle ProcessManager::Get_Free_Pid() const {
	for (uint16_t i = 0; i < static_cast<uint16_t>(process_table.size()); i += 1) {
		if (process_table[i] == nullptr) {
			return i + PID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}

kiv_os::THandle ProcessManager::Get_Free_Tid() const {
	for (uint16_t i = 0; i < static_cast<uint16_t>(thread_table.size()); i += 1) {
		if (thread_table[i] == nullptr) {
			return i + TID_RANGE_START;
		}
	}
	return NO_FREE_ID;
}

std::shared_ptr<Process> ProcessManager::Get_Process(const kiv_os::THandle pid) {
	return process_table[pid - PID_RANGE_START];
}

void ProcessManager::Add_Process(const std::shared_ptr<Process> process, const kiv_os::THandle pid) {
	process_table[pid] = process;
}

std::shared_ptr<Thread> ProcessManager::Get_Thread(const kiv_os::THandle tid) {
	return thread_table[tid - TID_RANGE_START];
}

auto ProcessManager::Add_Thread(const std::shared_ptr<Thread> thread, const kiv_os::THandle tid) -> void {
	thread_table[tid - TID_RANGE_START] = thread;
}

kiv_os::NOS_Error ProcessManager::Process_Syscall(kiv_hal::TRegisters& regs) {
	// Spustime jednotlive funkce podle operace
	auto operation = regs.rax.l;
	switch (static_cast<kiv_os::NOS_Process>(operation)) {
		case kiv_os::NOS_Process::Clone:
			return Perform_Clone(regs);

		case kiv_os::NOS_Process::Wait_For:
			return Perform_Wait_For(regs);

		case kiv_os::NOS_Process::Read_Exit_Code:
			return Perform_Read_Exit_Code(regs);

		case kiv_os::NOS_Process::Exit:
			return Perform_Process_Exit(regs);

		case kiv_os::NOS_Process::Shutdown:
			return Perform_Shutdown(regs);

		case kiv_os::NOS_Process::Register_Signal_Handler:
			return Perform_Register_Signal_Handler(regs);
	}
}

kiv_os::NOS_Error ProcessManager::Perform_Clone(kiv_hal::TRegisters& regs) {
	const auto operationType = static_cast<kiv_os::NClone>(regs.rcx.l);
	if (operationType == kiv_os::NClone::Create_Process) {
		return Create_Process(regs);
	}
	if (operationType == kiv_os::NClone::Create_Thread) {
		return Create_Thread(regs);
	}

	// Jinak vratime invalid argument
	return kiv_os::NOS_Error::Invalid_Argument;
}

kiv_os::THandle ProcessManager::Find_Parent_Pid() {
	const auto tid_handle = std::this_thread::get_id();

	// Pokud nelze tid prevest vratime invalid handle
	if (native_tid_to_kiv_handle.count(tid_handle) == 0) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak ziskame vlakno z tabulky a pid jeho procesu
	const auto tid = native_tid_to_kiv_handle[tid_handle];
	return thread_table[tid]->Get_Pid();
}

void ProcessManager::Dispatch_Process(std::shared_ptr<Process> process) {
	const auto main_thread_tid = process->Get_Process_Threads()[0];
	process->Set_Running();
}

kiv_os::NOS_Error ProcessManager::Create_Process(kiv_hal::TRegisters& regs) {
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
	auto lock = std::scoped_lock(mutex);
	// Nejprve provedeme check zda-li muzeme vytvorit novy proces - musi existovat volny pid a tid
	const auto pid = Get_Free_Pid();
	const auto tid = Get_Free_Tid();

	if (pid == NO_FREE_ID || tid == NO_FREE_ID) {
		return kiv_os::NOS_Error::Out_Of_Memory;
	}

	// Nastavime registry pro proces a predame je hlavnimu vlaknu
	auto process_context = kiv_hal::TRegisters();
	process_context.rax.x = std_in;
	process_context.rbx.x = std_out;

	// Vytvorime vlakno procesu, kde se spusti program
	auto main_thread = std::make_shared<Thread>(program, process_context, tid, pid);

	// Zjistime, zda-li vlakno, ve kterem se proces vytvari ma nejakeho rodice a nastavime ho (pokud existuje
	// jinak se nastavi invalid value)
	auto parent_process_pid = Find_Parent_Pid();
	auto process = std::make_shared<Process>(pid, tid, parent_process_pid, std_in, std_out);

	// Pridame proces a vlakno do tabulky
	Add_Process(process, pid);
	Add_Thread(main_thread, tid);

	// Spustime vlakno
	auto thread_handle = main_thread->Init();
	native_tid_to_kiv_handle[thread_handle] = tid;

	// Spustime proces
	Dispatch_Process(process);

	// Vratime success
	return kiv_os::NOS_Error::Success;
}
