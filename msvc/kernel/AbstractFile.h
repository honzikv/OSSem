#pragma once
#include <vector>
#include "../api/api.h"

/**
 * Abstraktni trida pro soubor v OS - muze tim byt cokoliv co splnuje toto api
 */
class AbstractFile {

public:
	
	/**
	 * Zapis do souboru - reference na buffer (vektor charu)
	 */
	virtual kiv_os::NOS_Error write(const std::vector<char>& buffer) = 0;

	/**
	 * Cteni z bufferu
	 * @param bytes - pocet bytu
	 */
	virtual kiv_os::NOS_Error read(uint32_t bytes) = 0;

	/**
	 * Presun na pozici v souboru
	 */
	virtual kiv_os::NOS_Error moveTo(uint32_t position) = 0;

	virtual kiv_os::NOS_Error close() = 0;

	
	virtual ~AbstractFile() = default;
};

