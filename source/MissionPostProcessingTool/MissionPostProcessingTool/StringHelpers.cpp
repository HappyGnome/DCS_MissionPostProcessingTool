
#include <string>
#include <iostream>

#include "StringHelpers.h"

bool str_helpers::TryAToF(const char* a, float& out) {
	char* pEnd;
	out = std::strtof(a, &pEnd);
	return *pEnd == '\0';
}

bool str_helpers::TryAToI(const char* a, int& out) {
	char* pEnd;
	out = (int)std::strtol(a, &pEnd,10);
	return *pEnd == '\0';
}

std::string str_helpers::BToS(int b) {
	if (b) return "true";
	else return "false";
}

bool str_helpers::TryAToB(const char* a, int& out) {
	
	if (std::string s(a); s == "true") out = 1;
	else if (s == "false") out = 0;
	else return false;

	return true;
}

bool str_helpers::TryAToNil(const char* a) {
	return std::string(a) == "nil";
}

std::string str_helpers::NilToS() {
	return "nil";
}