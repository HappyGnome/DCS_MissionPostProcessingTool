#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

#include "CsvFileTools.h"

//read csv data and leave it as a table on top of lua stack
std::vector<CsvEntry> CsvFileTools::SplitCSVLine(const std::string& line) {

	std::vector<CsvEntry> ret;

	bool escapeNext = false;
	bool literal = false;
	bool containsLiteral = false;
	bool newEntry = true;
	std::string nextEntry = "";

	for (const char& c : line) {
		containsLiteral |= literal;
		if (c == '\"' && newEntry) {
			literal = true;
			newEntry = false;
			continue;
		}
		else if (c == '\"' && literal) {
			escapeNext = true;
			literal = false;
			continue;
		}
		else if (c == '\"' && escapeNext) {
			escapeNext = false;
			literal = true;
			nextEntry += c;
			continue;
		}
		else if (c == ',' && !literal)
		{
			ret.emplace_back(nextEntry,containsLiteral);
			newEntry = true;
			escapeNext = false;
			containsLiteral = false;
			nextEntry = "";
		}
		else {
			nextEntry += c;
			escapeNext = false;
		}
	}
	ret.emplace_back(nextEntry, containsLiteral);
	return ret;
}

std::string CsvFileTools::EscapeCSVEntry(const std::string& input, bool forceEscape) {
	std::string ret = "";
	bool encloseInQuotes = forceEscape;
	for (const char& c : input) {
		if (c == '\"') {
			ret += "\"\"";
			encloseInQuotes = true;
		}
		else if (c == ',') {
			ret += c;
			encloseInQuotes = true;
		}
		else if (c != '\n' && c != '\r') ret += c;
	}

	if (encloseInQuotes) ret = "\"" + ret + "\"";
	return ret;
}