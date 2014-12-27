#include "readFile.h"

#include <fstream>
#include <stdexcept>
#include <sstream>

std::string readFile(const std::string& filename)
{
	std::ifstream t(filename);
	
	if (!t)
	{
		throw std::runtime_error("Nelze otevřít shader " + filename);
	}
	
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}
