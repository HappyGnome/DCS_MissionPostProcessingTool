#pragma once
#ifndef __MIZ_FILE_TOOLS__
#define __MIZ_FILE_TOOLS__
#include <string>

#include "Logger.h"
class MizFileTools {
	LoggerSection mLogger;
public:
	static const int INPUT_BUFSIZE = 1000000;

	explicit MizFileTools(std::shared_ptr<Logger> logger);

	bool backupFile(const std::string& mizPath) const;
	bool extractMissionData(const std::string& mizPath, std::string& output) const;
	bool overwriteMissionData(const std::string& mizPath, const std::string& newMizData) const;
};

#endif