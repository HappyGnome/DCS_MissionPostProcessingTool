#pragma once
#ifndef __MIZ_FILE_TOOLS__
#define __MIZ_FILE_TOOLS__
#include <string>
#include <filesystem>
#include "Logger.h"

extern "C" {
#include <zip.h>
}

class MizFileTools {
	std::shared_ptr<LoggerSection> mLogger;

	bool packDirectoryRecursive(zip_t* arch, const std::string& dirInMiz, const std::filesystem::path& dirToPack,
		const std::string& extension) const;
public:
	static const int INPUT_BUFSIZE = 1000000;

	explicit MizFileTools(std::shared_ptr<Logger> logger);

	bool backupFile(const std::string& mizPath, const std::filesystem::path& basePath) const;
	bool extractMissionData(const std::string& mizPath, std::string& output) const;
	bool overwriteMissionData(const std::string& mizPath, const std::string& newMizData) const;
	bool packFilesInMiz(const std::string& mizPath, const std::string& dirInMiz, const std::filesystem::path& dirToPack, const std::string& extension) const;
	bool unpackFilesInMiz(const std::string& mizPath, const std::string& dirInMiz, const std::filesystem::path& unpackTo, bool overwrite = false) const;
	bool clearDirInMiz(const std::string& mizPath, const std::string& dirInMiz) const;
};

#endif