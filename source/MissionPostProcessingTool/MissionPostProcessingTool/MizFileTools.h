#pragma once
#ifndef __MIZ_FILE_TOOLS__
#define __MIZ_FILE_TOOLS__
#include <string>

class MizFileTools {
public:
	static const int INPUT_BUFSIZE = 1000000;
	static bool backupFile(const std::string& mizPath);
	static bool extractMissionData(const std::string& mizPath, std::string& output);
	static bool overwriteMissionData(const std::string& mizPath, const std::string& newMizData);
};

#endif