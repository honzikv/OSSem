#pragma once

#include "../api/api.h"


/// <summary>
/// Abstraktni trida, ktera reprezentuje "soubor" v OS - tzn. cokoliv co umi cist a (nebo) do ceho lze psat
///	Defaultni implementace pro read a write vraci UnknownError a musi se prepsat pro spravnou funkcionalitu
/// </summary>
class AbstractFile {
public:
	virtual ~AbstractFile() = default;
	/// <summary>
	/// Precte data ze souboru do bufferu. Vychozi implementace vraci UnknownError
	/// </summary>
	/// <param name="targetBuffer">Buffer, do ktereho se data zapisuji</param>
	/// <param name="bytes">Pocet bytu, ktery se ma precist</param>
	/// <param name="bytesRead">Pocet prectenych bytu</param>
	/// <returns></returns>
	virtual kiv_os::NOS_Error read(char* targetBuffer, size_t bytes, size_t& bytesRead);

	/// <summary>
	/// Zapise data do souboru z bufferu. Vychozi implementace vraci UnknownError
	/// </summary>
	/// <param name="sourceBuffer">Buffer, ze ktereho se data ctou</param>
	/// <param name="bytes">Pocet bytu, ktery se ma zapsat</param>
	/// <param name="bytesWritten">Pocet zapsanych bytu po operaci</param>
	/// <returns></returns>
	virtual kiv_os::NOS_Error write(const char* sourceBuffer, size_t bytes, size_t& bytesWritten);

};
