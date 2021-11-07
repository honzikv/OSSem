#pragma once

#include "../api/api.h"
#include "Utils/Semaphore.h"

/// <summary>
/// Reprezentuje vlakno
/// </summary>
class Thread {

	/// <summary>
	/// Program, ktery bude vlakno vykonavat
	/// </summary>
	const kiv_os::TThread_Proc program;

	/// <summary>
	/// Semafor pro synchronizaci
	/// </summary>
	Semaphore semaphore;

	kiv_os::THandle tid;

	void threadFunc() {
		semaphore.acquire(); // Blokne vlakno

	}

public:
	/// <summary>
	/// Spusti vlakno
	/// </summary>
	void dispatch() {
		semaphore.release();
	}

	Thread(kiv_os::TThread_Proc program, kiv_hal::TRegisters context) : program(program) {
		
	}


	void init() {
		
	}
};

