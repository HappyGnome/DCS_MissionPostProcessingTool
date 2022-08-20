#include "MizFileTools.h"

#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

extern "C" {
#include <zip.h>
}

#include "Logger.h"

class ZipFileWrapper {
	zip_t* mArch = nullptr;
	bool mGood = false;
	std::shared_ptr<LoggerSection> mLogger;
public:
	ZipFileWrapper(std::shared_ptr<LoggerSection> logger, const std::string& mizPath) :mLogger(logger) {
		zip_error_t error = { 0,0 };
		zip_source_t* source = zip_source_win32a_create(mizPath.c_str(), 0, 0, &error);

		if (error.zip_err || error.sys_err) {
			mLogger->Error("libzip error code " + std::to_string(error.zip_err) + "/" + std::to_string(error.sys_err) + " when opening " + mizPath);
			return;
		}

		mArch = zip_open_from_source(source,ZIP_CREATE, &error);

		if (error.zip_err || error.sys_err) {
			mLogger->Error("libzip error code " + std::to_string(error.zip_err) + "/" + std::to_string(error.sys_err) + " when opening " + mizPath);
			return;
		}
		mGood = true;
	}

	ZipFileWrapper(const ZipFileWrapper&) = delete;

	~ZipFileWrapper() {
		if (mGood) zip_close(mArch);
	}

	ZipFileWrapper& operator=(const ZipFileWrapper&) = delete;

	zip_t* Arch() { return mArch; }
	bool IsGood() const { return mGood; }
};

//-----------------------------------------------------------------------

MizFileTools::MizFileTools(std::shared_ptr<Logger> logger) : mLogger(std::make_shared<LoggerSection>(logger,"MIZ IO")) {}

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
		mLogger->Error(error.message());
		return false;
	}
}

bool MizFileTools::extractMissionData(const std::string& mizPath, std::string& output) const{

	ZipFileWrapper zfw(mLogger, mizPath);
	if (!zfw.IsGood())return false;

	zip_file_t* missionFile = zip_fopen(zfw.Arch(), "mission", 0);
	if (missionFile == nullptr) return false;

	output = "";
	std::vector<char> buf(MizFileTools::INPUT_BUFSIZE, 0);

	while (true)
	{
		zip_int64_t readNum = zip_fread(missionFile, &buf[0], buf.size());
		if (readNum < 1) break;
		output += std::string(&buf[0], readNum);
	}
	zip_fclose(missionFile);

	mLogger->Info(mizPath + " read. Mission data length: " + std::to_string(output.length()));

	return true;
}

bool MizFileTools::overwriteMissionData(const std::string& mizPath, const std::string& newMizData) const{

	ZipFileWrapper zfw(mLogger, mizPath);
	if (!zfw.IsGood())return false;

	const char* buf = newMizData.c_str();


	if (zip_source_t* buf_src = zip_source_buffer(zfw.Arch(), buf, newMizData.length(), 0);
		zip_file_add(zfw.Arch(), "mission", buf_src, ZIP_FL_OVERWRITE) < 0) {

		zip_source_free(buf_src);
	}

	mLogger->Info(mizPath + " updated. New mission data length: " + std::to_string(newMizData.length()));
	return true;
}

bool MizFileTools::packDirectoryRecursive(zip_t* arch, const std::string& dirInMiz, const std::filesystem::path& dirToPack,
		const std::string& extension) const{

	std::string dirInMizEscaped = "";
	std::string dirInMizUnescaped = "";

	if (!dirInMiz.empty()) {
		if (dirInMiz.back() != '/'){
			dirInMizEscaped = dirInMiz + "/";
			dirInMizUnescaped = dirInMiz;
		}
		else {
			dirInMizEscaped = dirInMiz;
			dirInMizUnescaped = dirInMiz.substr(0,dirInMiz.length()-1);
		}
		if (zip_int64_t res = zip_dir_add(arch, dirInMizUnescaped.c_str(), 0);
			res < 0 && zip_get_error(arch)->zip_err != ZIP_ER_EXISTS) {
			mLogger->Error("Could not create " + dirInMizUnescaped + " in zip file.");
			return false;
		}
	}

	bool success = true;
	for (const auto& it : std::filesystem::directory_iterator(dirToPack)) {
		const auto& path = it.path();
		if (it.is_regular_file() && path.has_filename() && path.extension().string() == extension) {
			if (zip_source_t* file = zip_source_file_create(path.string().c_str(), 0, 0, 0);
				zip_file_add(arch, (dirInMizEscaped + path.filename().string()).c_str(), file, ZIP_FL_OVERWRITE) < 0) {
				mLogger->Warn("Could not create " + path.filename().string() + " in zip file.");
				zip_source_free(file);
				success = false;
			}
		}
		else if (it.is_directory()) {

			success = success && packDirectoryRecursive(arch, dirInMizEscaped + path.filename().string(), path, extension);
		}
	}
	return success;
}

bool MizFileTools::packFilesInMiz(const std::string& mizPath, const std::string& dirInMiz, const std::filesystem::path& dirToPack,
		const std::string& extension) const {
	ZipFileWrapper zfw(mLogger, mizPath);
	if (!zfw.IsGood())return false;
	

	return packDirectoryRecursive(zfw.Arch(), dirInMiz, dirToPack, extension);
}

bool UnpackZipFileToFile(zip_t* arch, const std::string& fileInMiz, const std::filesystem::path& unpackTo,const std::string& fileName, bool overwrite = false) {
	zip_file_t* missionFile = zip_fopen(arch, fileInMiz.c_str(), 0);
	if (missionFile == nullptr) return false;

	std::filesystem::path outPath = unpackTo / fileName;
	if (!overwrite && std::filesystem::exists(outPath))return true;

	std::filesystem::create_directories(unpackTo);
	
	auto ofile = std::ofstream(outPath, std::ios_base::trunc|std::ios_base::binary);
	if (!ofile.good()) return false;

	std::vector<char> buf(MizFileTools::INPUT_BUFSIZE, 0);

	while (true)
	{
		zip_int64_t readNum = zip_fread(missionFile, &buf[0], buf.size());
		if (readNum < 1) break;
		ofile.write(buf.data(), readNum);
	}
	zip_fclose(missionFile);
	return true;
}

bool MizFileTools::unpackFilesInMiz(const std::string& mizPath, const std::string& dirInMiz, const std::filesystem::path& unpackTo, bool overwrite) const {
	ZipFileWrapper zfw(mLogger, mizPath);
	if (!zfw.IsGood())return false;

	std::string dirInMizEscaped = dirInMiz;
	if (!dirInMiz.empty() && dirInMizEscaped.back() != '/') dirInMizEscaped += '/';

	bool success = true;

	int i = 0;
	while (true) {
		if (const char* path = zip_get_name(zfw.Arch(), i, 0);
				path != nullptr) {
			std::string sPath = std::string(path);
			auto fsPath = std::filesystem::path(sPath);

			if (sPath.rfind(dirInMizEscaped, 0) != std::string::npos && sPath.back() != '/') {

				std::string relPath = sPath.substr(dirInMizEscaped.length(), sPath.find_last_of('/') + 1 - dirInMizEscaped.length());
				
				if (relPath.rfind("..") != std::string::npos || relPath.rfind(":") != std::string::npos) {
					mLogger->Warn("Could not unpack " + std::string(path) + " from zip file. Illegal path.");
					success = false;
				}
				else if (!UnpackZipFileToFile(zfw.Arch(), path, unpackTo / std::filesystem::path(relPath), fsPath.filename().string(), overwrite)) {
					mLogger->Warn("Could not unpack " + std::string(path) + " from zip file.");
					success = false;
				}
			}
		}
		else if (zip_get_error(zfw.Arch())->zip_err != ZIP_ER_DELETED) {
			break;
		}
		i++;
	}
	return success;
}

bool MizFileTools::clearDirInMiz(const std::string& mizPath, const std::string& dirInMiz) const {
	ZipFileWrapper zfw(mLogger, mizPath);
	if (!zfw.IsGood())return false;

	std::string dirInMizEscaped = dirInMiz;
	if (!dirInMiz.empty() && dirInMizEscaped.back() != '/') dirInMizEscaped += '/';

	bool success = true;

	int i = 0;
	while (true) {
		if (const char* path = zip_get_name(zfw.Arch(), i, 0);
			path != nullptr) {
			std::string sPath = std::string(path);
			auto fsPath = std::filesystem::path(sPath);

			if (sPath.rfind(dirInMizEscaped, 0) != std::string::npos) {
				zip_delete(zfw.Arch(), i);
			}
		}
		else if (zip_get_error(zfw.Arch())->zip_err != ZIP_ER_DELETED) {
			break;
		}
		i++;
	}
	return success;
}