#pragma once
#ifndef __CSV_FILE_TOOLS__
#define __CSV_FILE_TOOLS__

#include <string>
#include <vector>

struct CsvEntry {
	std::string str;
	bool literal = false;

	explicit CsvEntry(const std::string &s, bool literal = false) :str(s),literal(literal) {}
};

class CsvFileTools {
public:
	static std::string EscapeCSVEntry(const std::string& input, bool forceEscape = false);
	static std::vector<CsvEntry> SplitCSVLine(const std::string& line);
};

#endif