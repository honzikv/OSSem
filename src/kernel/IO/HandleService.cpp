#include "HandleService.h"

kiv_os::THandle HandleService::Get_Empty_Handle() {
	auto lock = std::scoped_lock(mutex);
	if (handles.size() == Uint16tMax) {
		return kiv_os::Invalid_Handle;
	}

	// Iterujeme, dokud nenajdeme prazdny index
	while (handles.count(new_handle_id) > 0) {
		new_handle_id += 1;
	}

	const auto handle = new_handle_id; // ulozime si file descriptor
	handles.insert(handle);
	new_handle_id += 1; // Posuneme novy potencialni file descriptor o 1
	if (new_handle_id == kiv_os::Invalid_Handle) {
		new_handle_id = 0;
	}

	return handle;
}

void HandleService::Remove_Handle(const kiv_os::THandle handle) {
	auto lock = std::scoped_lock(mutex);
	handles.erase(handle);
}
