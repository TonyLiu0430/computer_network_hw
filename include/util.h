#pragma once
#include <string>

std::string trimLeft(const std::string& str, const std::string& charSet = " \t\n\r") {
	size_t start = str.find_first_not_of(charSet);
	return (start == std::string::npos) ? "" : str.substr(start);
}

std::string encodeSpecial(const std::string& str) {
	std::string encodedStr;
	for (char c : str) {
		if (c == '\n') {
			encodedStr += "\\n";
		}
		else if (c == '\\') {
			encodedStr += "\\\\";
		}
		else {
			encodedStr.push_back(c);
		}
	}
	return encodedStr;
}

