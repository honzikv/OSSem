#pragma once

#include "../../api/api.h"


/// <summary>
/// Abstraktni trida, ktera reprezentuje "soubor" v OS - tzn. cokoliv co umi cist a (nebo) do ceho lze psat
///	Defaultni implementace pro read a write vraci UnknownError a musi se prepsat pro spravnou funkcionalitu
/// </summary>
class IFile {

public:
	virtual ~IFile();
	/// <summary>
	/// Precte data ze souboru do bufferu. Vychozi implementace vraci UnknownError
	/// </summary>
	/// <param name="target_buffer">Buffer, do ktereho se data zapisuji</param>
	/// <param name="buffer_size">Pocet bytu, ktery se ma precist</param>
	/// <param name="bytes_read">Pocet prectenych bytu</param>
	/// <returns></returns>
	virtual kiv_os::NOS_Error Read(char* target_buffer, size_t buffer_size, size_t& bytes_read);

	/// <summary>
	/// Zapise data do souboru z bufferu. Vychozi implementace vraci UnknownError
	/// </summary>
	/// <param name="source_buffer">Buffer, ze ktereho se data ctou</param>
	/// <param name="buffer_size">Pocet bytu, ktery se ma zapsat</param>
	/// <param name="bytes_written">Pocet zapsanych bytu po operaci</param>
	/// <returns></returns>
	virtual kiv_os::NOS_Error Write(const char* source_buffer, size_t buffer_size, size_t& bytes_written);

	/// <summary>
	/// Zavre soubor
	/// </summary>
	virtual kiv_os::NOS_Error Close();

	/// <summary>
	/// Vyhledavani v souboru
	/// </summary>
	/// <param name="position">Presun na pozici</param>
	/// <param name="seek_type">Typ seeku</param>
	/// <returns></returns>
	virtual kiv_os::NOS_Error Seek(size_t position, kiv_os::NFile_Seek seek_type);
};
