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

#include "main.h"

#include "Config.h"
#include "MizFileTools.h"
#include "LuaWrapper.h"
#include "Logger.h"

constexpr const char* coreLuaPath = "scripts\\core\\core.lua";
constexpr const char* kneeboardLuaPath = "scripts\\kneeboards\\getAirframes.lua";
constexpr const char* inputKneeboardDir = "input\\kneeboards";
constexpr const char* configLuaPath = "config_kneeboard.lua";
constexpr const char* backupsPath = "backups\\";

struct ErrorCodes {
	static constexpr int LOAD_CONFIG = -1;
	static constexpr int SAVE_CONFIG = -2;
	static constexpr int EXTRACTING_MIZ = -3;
	static constexpr int SAVING_MIZ = -4;
	static constexpr int EXTRACTING_AIRFRAMES = -5;
	static constexpr int EXTRACTING_DIRECTORIES = -6;
	static constexpr int SYNCING_KNEEBOARDS = -9;
	static constexpr int UPDATING_MIZ = -7;
	static constexpr int FIND_SCRIPTS = -8;
	static constexpr int UNKNOWN = -10;
};

class MainContext
{
	std::shared_ptr<Logger> mLogger;
	std::shared_ptr <MizFileTools> mMiz;
	std::filesystem::path mExeDir;
	AppConfig mConfig;
public:	

	std::shared_ptr <MizFileTools> Miz() const { return mMiz; }

	MainContext() {
		char buf[1024];
		GetModuleFileNameA(nullptr, buf, sizeof(buf));
		std::filesystem::path exeDir(buf);
		mExeDir = exeDir.remove_filename();
		std::filesystem::path logPath = MakeRelativeToExe("MissionPostProcessingTool_Kneeboard.log");

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
	
	bool GetClientAirframeTypes(const std::string& missionData, std::vector<std::string>& output) const {

		LuaWrapper lw(mLogger);

		std::filesystem::path luaPath = MakeRelativeToExe(kneeboardLuaPath);

		lw.RunScript(missionData);//add mission global object

		if (!lw.RunFile(luaPath.string())) return false;

		if (!lw.GetGlobalFunction("clientAirframeList")) return false;
		if (!lw.CallFunction(0,1)) return false;
		if (std::map<std::string, bool> map; lw.ReadFromTable<bool>(map)) {
			output.clear();
			for (auto it = map.cbegin(); it != map.cend(); ++it) {
				output.push_back(it->first);
			}
			return true;
		}
		else return false;
	}

	bool EnsureMissionKneeboardFolders(const std::string& missionFileName, const std::vector<std::string>& airframeNames, std::filesystem::path &mizKbDir) const {
		
		mizKbDir = MakeRelativeToExe(inputKneeboardDir)/std::filesystem::path(missionFileName).filename();
		if (!std::filesystem::exists(mizKbDir)) std::filesystem::create_directories(mizKbDir);

		std::filesystem::create_directories(mizKbDir / "IMAGES");

		for (const std::filesystem::path& it : airframeNames) {
			if (std::filesystem::path(it).has_parent_path()) continue;
			std::filesystem::create_directories(mizKbDir / it / "IMAGES");
		}
		return true;
	}

	bool SyncKneeboards(const std::string& missionFileName, const std::filesystem::path& kneeboardsPath, bool clearKneeboards) const{
		bool success = true;
		std::string kbMizpath = "KNEEBOARD";
		if (clearKneeboards) success &= mMiz->clearDirInMiz(missionFileName, kbMizpath);

		std::vector<std::string> extensions = { ".jpg",".jpeg", ".png" };

		for (const auto& e : extensions) {
			success &= mMiz->packFilesInMiz(missionFileName, kbMizpath, kneeboardsPath, e);
		}

		success &= mMiz->unpackFilesInMiz(missionFileName, kbMizpath, kneeboardsPath);
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

	bool GetConfig(){		
		std::filesystem::path path = MakeRelativeToExe(configLuaPath);
		if (!std::filesystem::exists(path)) return true;//nothing to load

		LuaWrapper lw(mLogger);

		if (!lw.RunFile(path.string())) return false;

		lua_getglobal(lw.L(), "config");

		if (std::map<std::string, std::string> map; lw.ReadFromTable<std::string>(map)) mConfig.ApplyMap(map);

		lua_pop(lw.L(), 1);
		return true;
	}

	bool SaveConfig() {
		std::filesystem::path path = MakeRelativeToExe(configLuaPath);
		std::filesystem::path corePath = MakeRelativeToExe(coreLuaPath);

		LuaWrapper lw(mLogger);

		if (!lw.RunFile(corePath.string())) return false;

		lua_getglobal(lw.L(), "core");
		if (!lua_istable(lw.L(), -1)) return false;
		lua_getfield(lw.L(), -1, "luaObjToLuaString");
		if (!lua_isfunction(lw.L(), -1)) return false;

		//function args
		lua_createtable(lw.L(),0,0);
		
		std::map<std::string, std::string,std::less<std::string>> map; 
		mConfig.AddToMap(map);
		lw.WriteToTable(map);

		lua_pushstring(lw.L(), "config");

		lua_pcall(lw.L(), 2, 1, 0);

		if (lua_isstring(lw.L(), -1)) {
			std::string  fileContent = lua_tostring(lw.L(), -1);
			lua_pop(lw.L(), 2); //core & return value

			std::ofstream csv(path, std::ios_base::trunc);

			/*if (!csv.good()) return false;
			csv << "-- REMINDER: Use lua escape sequences in string variables!" << std::endl
				<< "--           e.g. [\"" << AppConfig::EditorExePath_key << "\"] = \"C:\\MyApp.exe\"    -- Bad" << std::endl
				<< "--                [\"" << AppConfig::EditorExePath_key << "\"] = \"C:\\\\MyApp.exe\"   -- OK" << std::endl;*/
			csv << fileContent;
			return true;
		}
		else lua_pop(lw.L(), 2); //core & return value

		return false;
	}

	std::string GetFromList(const std::vector<std::string>& options, const std::string& prompt = "") {
		if (options.size() == 0)return "";
		else if (options.size() == 1)return options[0];

		if (prompt != "") std::cout << prompt << " Use <Tab> to cycle: ";
		int at = 0;
		std::cout << options[at];
		while (true) {
			int c = _getch();
			if (c == '\t') {
				for (const char& c : options[at]) std::cout << "\b \b";
				at = ++at % options.size();
				std::cout << options[at];
			}
			if (c == 0) continue;
			if (c == '\n' || c == '\r') break;
		}
		return options[at];
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
	if(!mc.GetConfig()) return mc.OnError("while loading config","",ErrorCodes::LOAD_CONFIG);
	if (!mc.SaveConfig()) return mc.OnError("while saving config", "", ErrorCodes::SAVE_CONFIG);


	std::string missionFileContent;

	try {
		if (!mc.Miz()->extractMissionData(missionFilePath, missionFileContent)) return mc.OnError("while extracting mission data", "", ErrorCodes::EXTRACTING_MIZ);

		std::vector<std::string> airframes;
		std::filesystem::path mizKbDir;

		if(!mc.GetClientAirframeTypes(missionFileContent, airframes)) return mc.OnError("while extracting airframe list", "", ErrorCodes::EXTRACTING_AIRFRAMES);
		if(!mc.EnsureMissionKneeboardFolders(missionFilePath,airframes,mizKbDir)) return mc.OnError("while creating directories", "", ErrorCodes::EXTRACTING_DIRECTORIES);

		mc.Miz()->backupFile(missionFilePath,mc.MakeRelativeToExe(backupsPath));

		bool cleanSync = mc.GetFromList({ "Keep","Clear" }, "Clear kneeboards from miz before sync?") == "Clear";
		if (!mc.SyncKneeboards(missionFilePath,mizKbDir,cleanSync))return mc.OnError("while syncing kneeboards", "", ErrorCodes::SYNCING_KNEEBOARDS);
		

	}
	catch (std::exception ex){
		return mc.OnError("unexpectedly", ex.what(), ErrorCodes::UNKNOWN);
	}

	return 0;
}