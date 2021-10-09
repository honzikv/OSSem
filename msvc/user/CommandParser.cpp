#include "CommandParser.h"

const char* ParseException::what() const throw() {
	return err.c_str();
}
