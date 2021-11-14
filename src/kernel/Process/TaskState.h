#pragma once
#include <cstdint>

/// <summary>
/// Stav procesu/vlakna
/// </summary>
enum class TaskState : uint8_t {
	Ready, // Nove vytvorene vlakno / proces
	Running, // Stav, kdy vlakno / proces aktivne bezi
	Finished, // Vlakno/proces dobehli a bude se odstranat z tabulky
	Terminated, // Vlakno / proces nedobehli a byli nasilne ukonceny
};