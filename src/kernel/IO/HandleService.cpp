#include "HandleService.h"

kiv_os::THandle HandleService::CreateHandle(const HANDLE native_handle) {
	auto lock = std::scoped_lock(mutex);
	if (handles.size() == UINT16_T_MAX) {
		return kiv_os::Invalid_Handle;
	}

	// Iterujeme, dokud nenajdeme prazdny index
	while (handles.count(new_handle_id) > 0) {
		new_handle_id += 1;
	}

	const auto handle = new_handle_id; // ulozime si file descriptor
	handles[handle] = native_handle;
	new_handle_id += 1; // Posuneme novy potencialni file descriptor o 1
	if (new_handle_id == kiv_os::Invalid_Handle) {
		new_handle_id = 0;
	}

	return handle;
}

kiv_os::THandle HandleService::CreateEmptyHandle() {
	return CreateHandle(nullptr);
}

HANDLE HandleService::GetHandle(const kiv_os::THandle handle) {
	auto lock = std::scoped_lock(mutex);
	if (handles.count(handle) == 0) {
		return INVALID_HANDLE_VALUE;
	}

	return handles.at(handle);
}

void HandleService::RemoveHandle(const kiv_os::THandle handle) {
	auto lock = std::scoped_lock(mutex);
	handles.erase(handle);
}
