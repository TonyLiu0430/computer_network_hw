#pragma once
#include <string>

std::string trimLeft(const std::string& str, const std::string& charSet = " \t\n\r") {
	size_t start = str.find_first_not_of(charSet);
	return (start == std::string::npos) ? "" : str.substr(start);
}