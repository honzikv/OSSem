#pragma once

#include "Shell/Shell.h"
#if IS_DEBUG

/// <summary>
/// Jednoducha trida pro debugovani shellu
/// </summary>
class TestRunner {

public:
	/// <summary>
	/// Spusteni testu
	/// </summary>
	static void Run_Tests();
};
#endif
