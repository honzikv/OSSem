#pragma once
#include <mutex>

/// <summary>
/// Implementace semaforu, protoze C++ to ma az od C++20 ...
/// </summary>
class Semaphore {

	/// <summary>
	/// Mutex pro zablokovani pri vstupu
	/// </summary>
	std::mutex mutex;

	/// <summary>
	/// Condition variable pro uspani vlakna
	/// </summary>
	std::condition_variable condition_variable = {};

	/// <summary>
	/// Pocet vstupu soucasne
	/// </summary>
	size_t count = 0;

public:

	/// <summary>
	/// Ziskani pristupu - blokuje, pokud je count = 0
	/// </summary>
	void Acquire();

	/// <summary>
	/// Uvolneni pristupu - zvysi count o 1 a notifikuje jakekoliv spici vlakno nad condition variable
	/// </summary>
	void Release();

	/// <summary>
	/// Konstruktor pro semafor, ktery automaticky blokne pri acquire() a jine vlakno ho odblokne pomoci release()
	/// </summary>
	Semaphore() = default;

	/// <summary>
	/// Konstruktor pro specifikaci poctu simultanlnich vstupu
	///	count = 1 - slouzi k blokaci kriticke sekce
	/// </summary>
	/// <param name="count">Pocet simultanich vstupu</param>
	explicit Semaphore(const size_t count);
};

