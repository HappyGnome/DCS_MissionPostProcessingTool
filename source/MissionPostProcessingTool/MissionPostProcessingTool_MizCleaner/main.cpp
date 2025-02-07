/*
*	COPYRIGHT HappyGnome (https://github.com/HappyGnome) 2025 
*/

#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

#include "conio.h"
#include<shlwapi.h>
#include<shlobj.h>
//#include <Libloaderapi.h>

extern "C" {
	#include <zip.h>
//	#include "lua.h"
//	#include "lauxlib.h"
//	#include "lualib.h"
}

//#include "main.h"

//#include "Config.h"
#include "MizFileTools.h"
//#include "LuaWrapper.h"
#include "Logger.h"

//constexpr const char* coreLuaPath = "scripts\\core\\core.lua";
constexpr const char* backupsPath = "backups\\";

struct ErrorCodes {
	static constexpr int LOAD_CONFIG = -1;
	static constexpr int SAVE_CONFIG = -2;
	static constexpr int EXTRACTING_MIZ = -3;
	static constexpr int SAVING_MIZ = -4;
	static constexpr int CLEARING_CONFIG = -5;
	static constexpr int CLEARING_TRACK = -6;
	static constexpr int UPDATING_MIZ = -7;
	static constexpr int UNKNOWN = -10;
};

class MainContext
{
	std::shared_ptr<Logger> mLogger;
	std::shared_ptr <MizFileTools> mMiz;
	std::filesystem::path mExeDir;
	//AppConfig mConfig;
public:	

	std::shared_ptr <MizFileTools> Miz() const { return mMiz; }

	MainContext() {
		char buf[1024];
		GetModuleFileNameA(nullptr, buf, sizeof(buf));
		std::filesystem::path exeDir(buf);
		mExeDir = exeDir.remove_filename();
		std::filesystem::path logPath = MakeRelativeToExe("MissionPostProcessingTool_MizCleaner.log");

		mLogger = std::make_shared<Logger>(logPath.string());
		mMiz = std::make_shared<MizFileTools>(mLogger);
	}

	void MakeRelativeToExe(std::filesystem::path& path) const{
		if (path.is_absolute()) return;

		path = mExeDir / path;
	}
	std::filesystem::path MakeRelativeToExe(const std::string& path) const{
		auto ret = std::filesystem::path(mExeDir);
		return ret.append(path);
	}
	
	bool ClearMizEmbeddedConfig(const std::string& missionFileName) const{
		bool success = true;
		std::string kbMizpath = "Config";

		success &= mMiz->clearDirInMiz(missionFileName, kbMizpath);

		return success;
	}


	bool ClearTrackData(const std::string& missionFileName) const{
		bool success = true;
		std::vector<std::string> mizPaths = { "track","track_data" };
		
		for (auto& path : mizPaths) {
			success &= mMiz->clearDirInMiz(missionFileName, path);
		}

		return success;
	}

	int OnError(const std::string& failedAction, const std::string& toLog, int code) const{
		std::string printMessage = "Error occurred " + failedAction + ". ";
		std::cout << printMessage << std::endl
				  << "See log for details." << std::endl;

		std::string logMessage = printMessage + "\nStopcode " + std::to_string(code) + ". ";
		if (!toLog.empty()) logMessage += "\nDetails: " + toLog;
		mLogger->Error(logMessage);
		return code;
	}

};

int main(int argc, char* argv[]) {

	std::string missionFilePath;
	if (argc < 2) {
		std::cout << "Enter path to .miz file: ";
		std::getline(std::cin,missionFilePath);
		if (!missionFilePath.empty() && missionFilePath.front() == '\"')missionFilePath = missionFilePath.substr(1, missionFilePath.length() - 1);
		if (!missionFilePath.empty() && missionFilePath.back() == '\"')missionFilePath = missionFilePath.substr(0, missionFilePath.length() - 1);
	}
	else missionFilePath = std::string(argv[1]);

	MainContext mc;	

	try {

		mc.Miz()->backupFile(missionFilePath,mc.MakeRelativeToExe(backupsPath));

		if(!mc.ClearMizEmbeddedConfig(missionFilePath)) return mc.OnError("clearing embedded config", "", ErrorCodes::CLEARING_CONFIG);
		if(!mc.ClearTrackData(missionFilePath)) return mc.OnError("clearing embedded track data", "", ErrorCodes::CLEARING_TRACK);

	}
	catch (std::exception ex){
		return mc.OnError("unexpectedly", ex.what(), ErrorCodes::UNKNOWN);
	}

	return 0;
}
