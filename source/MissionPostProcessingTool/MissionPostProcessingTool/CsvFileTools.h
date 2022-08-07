#pragma once
#ifndef __CSV_FILE_TOOLS__
#define __CSV_FILE_TOOLS__

#include <string>
#include <vector>

#include "Logger.h"

struct CsvEntry {
	std::string str;
	bool literal = false;

	explicit CsvEntry(const std::string &s, bool literal = false) :str(s),literal(literal) {}
};

class CsvFileTools {
	LoggerSection mLogger;
public:
	explicit CsvFileTools(std::shared_ptr<Logger> logger);
	std::string EscapeCSVEntry(const std::string& input, bool forceEscape = false) const;
	std::vector<CsvEntry> SplitCSVLine(const std::string& line) const;
};

#endif