#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

#include "conio.h"
#include<shlwapi.h>
#include<shlobj.h>
#include <Libloaderapi.h>

extern "C" {
#include <zip.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "MizFileTools.h"
#include "LuaWrapper.h"
#include "Logger.h"

constexpr const char* outputFile = "diff.lua";
constexpr const char* coreLuaPath = "scripts\\core\\core.lua";

struct ErrorCodes {
	static constexpr int LOAD_CONFIG = -1;
	static constexpr int SAVE_CONFIG = -2;
	static constexpr int EXTRACTING_MIZ = -3;
	static constexpr int COMPARING_MIZ = -4;
	static constexpr int EXTRACTING_CSV = -5;
	static constexpr int SAVING_FILE = -6;
	static constexpr int UPDATING_MIZ = -7;
	static constexpr int FIND_SCRIPTS = -8;
	static constexpr int EDITING_CSV = -9;
	static constexpr int UNKNOWN = -10;
};

class MainContext
{
	std::shared_ptr<Logger> mLogger;
	std::shared_ptr <MizFileTools> mMiz;
	std::filesystem::path mExeDir;
public:

	std::shared_ptr <MizFileTools> Miz() const { return mMiz; }

	MainContext() {
		char buf[1024];
		GetModuleFileNameA(nullptr, buf, sizeof(buf));
		std::filesystem::path exeDir(buf);
		mExeDir = exeDir.remove_filename();
		std::filesystem::path logPath = MakeRelativeToExe("MissionDiffTool.log");

		mLogger = std::make_shared<Logger>(logPath.string());
		mMiz = std::make_shared<MizFileTools>(mLogger);
	}

	void MakeRelativeToExe(std::filesystem::path& path) const {
		if (path.is_absolute()) return;

		path = mExeDir / path;
	}
	std::filesystem::path MakeRelativeToExe(const std::string& path) const {
		auto ret = std::filesystem::path(mExeDir);
		return ret.append(path);
	}
	bool LuaGetDiff(const std::string& missionData1, const std::string& missionData2, std::string& diffData) const {

		LuaWrapper lw(mLogger);

		std::filesystem::path corePath = MakeRelativeToExe(coreLuaPath);

		lw.RunScript(missionData2);//add mission global object
		lw.RunScript("mission2 = mission");
		lw.RunScript(missionData1);//add mission global object

		if (!lw.RunFile(corePath.string())) return false;

		lua_getglobal(lw.L(), "core");
		if (!lua_istable(lw.L(), -1)) {
			lua_pop(lw.L(), 1);
			return false;
		}

		lua_getfield(lw.L(), -1, "luaDiff");
		if (!lua_isfunction(lw.L(), -1)) {
			lua_pop(lw.L(), 2);
			return false;
		}

		lua_getglobal(lw.L(), "mission");
		lua_getglobal(lw.L(), "mission2");

		if (!lw.CallFunction(2, 1))return false;

		if (!lua_isstring(lw.L(), -1))return false;
		diffData = lua_tostring(lw.L(), -1);

		lua_pop(lw.L(), 2); //result and core
		return true;
	}

	int OnError(const std::string& failedAction, const std::string& toLog, int code) const {
		std::string printMessage = "Error occurred " + failedAction + ". ";
		std::cout << printMessage << std::endl
			<< "See log for details." << std::endl;

		std::string logMessage = printMessage + "\nStopcode " + std::to_string(code) + ". ";
		if (!toLog.empty()) logMessage += "\nDetails: " + toLog;
		mLogger->Error(logMessage);
		return code;
	}

	std::string StripQuotes(std::string& s) {
		if (!s.empty() && s.front() == '\"')s = s.substr(1, s.length() - 1);
		if (!s.empty() && s.back() == '\"')s = s.substr(0, s.length() - 1);
		return s;
	}
};

int main(int argc, char* argv[]) {

	MainContext mc;

	std::string missionFilePath, missionFilePath2;
	if (argc < 2) {
		std::cout << "Enter path to .miz file: ";
		std::getline(std::cin, missionFilePath);
		mc.StripQuotes(missionFilePath);
	}
	else missionFilePath = std::string(argv[1]);

	if (argc < 3) {
		std::cout << "Enter path to second .miz file: ";
		std::getline(std::cin, missionFilePath2);
		mc.StripQuotes(missionFilePath2);
	}
	else missionFilePath2 = std::string(argv[2]);

	std::string missionFileContent, missionFileContent2;

	try {
		if (!mc.Miz()->extractMissionData(missionFilePath, missionFileContent)) return mc.OnError("while extracting mission data", "", ErrorCodes::EXTRACTING_MIZ);
		if (!mc.Miz()->extractMissionData(missionFilePath2, missionFileContent2)) return mc.OnError("while extracting mission2 data", "", ErrorCodes::EXTRACTING_MIZ);

		std::string result;

		if(!mc.LuaGetDiff(missionFileContent,missionFileContent2,result))return mc.OnError("while comparing data", "", ErrorCodes::COMPARING_MIZ);

		std::ofstream ofile(mc.MakeRelativeToExe(outputFile), std::ios_base::trunc);
		if (!ofile.good()) return mc.OnError("while saving file", "", ErrorCodes::SAVING_FILE);

		ofile << result;
	}
	catch (std::exception ex) {
		return mc.OnError("unexpectedly", ex.what(), ErrorCodes::UNKNOWN);
	}

	return 0;
}