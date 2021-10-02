#include "handles.h"

#include <map>
#include <mutex>
#include <random>

std::map<kiv_os::THandle, HANDLE> Handles;
std::mutex Handles_Guard;
kiv_os::THandle Last_Handle = 0;

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, 6);



kiv_os::THandle Convert_Native_Handle(const HANDLE hnd) {
	std::lock_guard<std::mutex> guard(Handles_Guard);

	Last_Handle += dis(gen);	//vygenerujeme novy interni handle s nahodnou hodnotou

	Handles.insert(std::pair<kiv_os::THandle, HANDLE>(Last_Handle, hnd));

	return Last_Handle;
}

HANDLE Resolve_kiv_os_Handle(const kiv_os::THandle hnd) {
	std::lock_guard<std::mutex> guard(Handles_Guard);

	auto resolved = Handles.find(hnd);
	if (resolved != Handles.end()) {
		return resolved->second;
	}
	else
		return INVALID_HANDLE_VALUE;
}

bool Remove_Handle(const kiv_os::THandle hnd) {
	std::lock_guard<std::mutex> guard(Handles_Guard);

	return Handles.erase(hnd) == 1;
}