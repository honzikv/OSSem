#include "TaskIdService.h"

kiv_os::THandle TaskIdService::Get_Free_Pid() {
	auto lock = std::scoped_lock(mutex);

	// Pokud je pocet pidu vycerpany vratime Invalid_Handle
	if (allocated_pids.size() == MaxPids) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak projizdime dokud nenajdeme pid
	while (allocated_pids.count(last_free_pid) > 0) {
		last_free_pid += 1;
	}

	const auto pid = last_free_pid;
	allocated_pids.insert(pid);
	last_free_pid += 1;

	// Check jestli jsme neprekrocili rozsah
	if (last_free_pid == PidRangeEnd) {
		last_free_pid = PidRangeStart;
	}

	return pid;
}

kiv_os::THandle TaskIdService::Get_Free_Tid() {
	auto lock = std::scoped_lock(mutex);

	// Pokud je pocet tidu vycerpany vratime Invalid_Handle
	if (allocated_tids.size() == MaxTids) {
		return kiv_os::Invalid_Handle;
	}

	// Jinak projizdime dokud nenajdeme tid
	while (allocated_tids.count(last_free_tid) > 0) {
		last_free_tid += 1;
	}

	const auto tid = last_free_tid;
	allocated_tids.insert(tid);
	last_free_tid += 1;

	// Check jestli jsme neprekrocili rozsah
	if (last_free_tid == TidRangeEnd) {
		last_free_tid = TidRangeStart;
	}

	return tid;
}

void TaskIdService::Remove_Pid(const kiv_os::THandle pid) {
	auto lock = std::scoped_lock(mutex);
	allocated_pids.erase(pid);
}

void TaskIdService::Remove_Tid(const kiv_os::THandle tid) {
	auto lock = std::scoped_lock(mutex);
	allocated_tids.erase(tid);
}

HandleType TaskIdService::Get_Handle_Type(const kiv_os::THandle id) {
	if (id >= PidRangeStart && id < PidRangeEnd) {
		// Pokud je handle mezi 0 - PID_RANGE_END jedna se o proces
		return HandleType::Process;
	}
	// Jinak musi byt handle mezi TID_RANGE_START a TID_RANGE_END
	return id >= TidRangeStart && id < TidRangeEnd ? HandleType::Thread : HandleType::INVALID;
}
