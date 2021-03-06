#pragma once
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "../../api/api.h"

/// <summary>
/// Singleton pro vytvareni handlu
/// </summary>
class HandleService {

public:
	/// <summary>
	/// Z nejakeho duvodu numeric limits ve windows nefunguji ...
	/// </summary>
	static constexpr uint16_t Uint16tMax = 0xffff;

	static HandleService& Get() {
		static HandleService instance;
		return instance;
	}

private:
	HandleService() = default;
	~HandleService() = default;
	HandleService(const HandleService&) = delete;
	HandleService& operator=(const HandleService&) = delete;

	/// <summary>
	/// Potencialni id noveho handlu
	/// </summary>
	uint16_t new_handle_id = 0;

	/// <summary>
	/// Zabrane handles
	/// </summary>
	std::unordered_set<uint16_t> handles;

	/// <summary>
	/// Mutex pro pristup do tabulky
	/// </summary>
	std::mutex mutex;

public:
	/// <summary>
	/// Vytvori novy empty handle - pouziva se pro file descriptory
	/// </summary>
	/// <returns></returns>
	kiv_os::THandle Get_Empty_Handle();

	/// <summary>
	/// Odstrani handle z OS
	/// </summary>
	/// <param name="handle"></param>
	void Remove_Handle(kiv_os::THandle handle);

};
