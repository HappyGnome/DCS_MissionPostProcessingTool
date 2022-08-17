#include "MizFileTools.h"

#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

extern "C" {
#include <zip.h>
}

#include "Logger.h"

MizFileTools::MizFileTools(std::shared_ptr<Logger> logger) : mLogger(logger,"MIZ IO") {}

bool MizFileTools::backupFile(const std::string& mizPath, const std::filesystem::path& basePath) const {

	auto inputPath = std::filesystem::path(mizPath);

	if (!std::filesystem::exists(basePath)) std::filesystem::create_directory(basePath);

	std::filesystem::path bkpPath = basePath/(inputPath.filename().string() +".bkp");

	std::filesystem::path outPath;
	for (int i = 0;; i++) {
		outPath = bkpPath;
		outPath += std::to_string(i);

		if (!std::filesystem::exists(outPath)) break;
	}

	std::error_code error;
	std::filesystem::copy(inputPath, outPath, error);

	if (error.value() == 0) return true;
	else {
		mLogger.Error(error.message());
		return false;
	}
}

bool MizFileTools::extractMissionData(const std::string& mizPath, std::string& output) const{

	zip_error_t error = {0,0};
	zip_source_t* source = zip_source_win32a_create(mizPath.c_str(), 0, 0, &error);

	if (error.zip_err || error.sys_err) {
		mLogger.Error("libzip error code " + std::to_string(error.zip_err) +"/" + std::to_string(error.sys_err) + " when opening " + mizPath);
		return false;
	}

	zip_t* arch = zip_open_from_source(source, ZIP_RDONLY, &error);

	if (error.zip_err || error.sys_err) {
		mLogger.Error("libzip error code " + std::to_string(error.zip_err) + "/" + std::to_string(error.sys_err) + " when opening " + mizPath);
		return false;
	}

	zip_file_t* missionFile = zip_fopen(arch, "mission", 0);

	output = "";
	std::vector<char> buf(MizFileTools::INPUT_BUFSIZE, 0);

	while (true)
	{
		zip_int64_t readNum = zip_fread(missionFile, &buf[0], buf.size());
		if (readNum < 1) break;
		output += std::string(&buf[0], readNum);
	}
	zip_fclose(missionFile);
	zip_close(arch);

	mLogger.Info(mizPath + " read. Mission data length: " + std::to_string(output.length()));

	return true;
}

bool MizFileTools::overwriteMissionData(const std::string& mizPath, const std::string& newMizData) const{

	zip_error_t error = {0,0};
	zip_source_t* source = zip_source_win32a_create(mizPath.c_str(), 0, 0, &error);

	if (error.zip_err || error.sys_err) {
		mLogger.Error("libzip error code " + std::to_string(error.zip_err) + "/" + std::to_string(error.sys_err) + " when opening " + mizPath);
		return false;
	}

	zip_t* arch = zip_open_from_source(source, 0, &error);

	if (error.zip_err || error.sys_err) {
		mLogger.Error("libzip error code " + std::to_string(error.zip_err) + "/" + std::to_string(error.sys_err) + " when opening " + mizPath);
		return false;
	}

	const char* buf = newMizData.c_str();


	if (zip_source_t* buf_src = zip_source_buffer(arch, buf, newMizData.length(), 0);
		zip_file_add(arch, "mission", buf_src, ZIP_FL_OVERWRITE) < 0) {

		zip_source_free(buf_src);
	}

	zip_close(arch);

	mLogger.Info(mizPath + " updated. New mission data length: " + std::to_string(newMizData.length()));
	return true;
}