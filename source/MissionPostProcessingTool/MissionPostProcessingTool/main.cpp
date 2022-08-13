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
#include "CsvFileTools.h"
#include "MizFileTools.h"
#include "LuaWrapper.h"
#include "Logger.h"

constexpr const char* mainLuaDir = "scripts\\";
constexpr const char* coreLuaPath = "scripts\\core\\core.lua";
constexpr const char* inputCsvDir = "input\\";
constexpr const char* configLuaPath = "config.lua";
constexpr const char* backupsPath = "backups\\";

struct ErrorCodes {
	static constexpr int LOAD_CONFIG = -1;
	static constexpr int SAVE_CONFIG = -2;
	static constexpr int EXTRACTING_MIZ = -3;
	static constexpr int SAVING_MIZ = -4;
	static constexpr int EXTRACTING_CSV = -5;
	static constexpr int SAVING_CSV = -6;
	static constexpr int UPDATING_MIZ = -7;
	static constexpr int FIND_SCRIPTS = -8;
	static constexpr int EDITING_CSV = -9;
	static constexpr int UNKNOWN = -10;
};

class MainContext
{
	std::shared_ptr<Logger> mLogger;
	std::shared_ptr <MizFileTools> mMiz;
	std::shared_ptr <CsvFileTools> mCsv;
	std::filesystem::path mExeDir;
	AppConfig mConfig;
public:	

	std::shared_ptr <MizFileTools> Miz() const { return mMiz; }
	std::shared_ptr <CsvFileTools> Csv() const { return mCsv; }

	MainContext() {
		char buf[1024];
		GetModuleFileNameA(nullptr, buf, sizeof(buf));
		std::filesystem::path exeDir(buf);
		mExeDir = exeDir.remove_filename();
		std::filesystem::path logPath = MakeRelativeToExe("MissionPostProcessingTool.log");

		mLogger = std::make_shared<Logger>(logPath.string());
		mMiz = std::make_shared<MizFileTools>(mLogger);
		mCsv = std::make_shared<CsvFileTools>(mLogger);
	}

	void MakeRelativeToExe(std::filesystem::path& path) const{
		if (path.is_absolute()) return;

		path = mExeDir / path;
	}
	std::filesystem::path MakeRelativeToExe(const std::string& path) const{
		auto ret = std::filesystem::path(mExeDir);
		return ret.append(path);
	}
	bool LuaApplyScript(const std::filesystem::path& csvPath, const std::filesystem::path& mainLuaPath, const std::string& missionData, std::string& newMissionData) const{

		LuaWrapper lw(mLogger,mCsv);

		std::filesystem::path path = std::filesystem::absolute(mainLuaPath);
		std::filesystem::path corePath = MakeRelativeToExe(coreLuaPath);

		lw.RunScript(missionData);//add mission global object

		if (!lw.RunFile(corePath.string())) return false;
		if (!lw.RunFile(path.string())) return false;

		return lw.RunCsvHandler("applyCsv", std::filesystem::absolute(csvPath).string(), newMissionData);
	}

	bool LuaExtractScript(const std::filesystem::path& csvPath, const std::filesystem::path& mainLuaPath, const std::string& missionData) const{

		LuaWrapper lw(mLogger,mCsv);

		std::filesystem::path path = std::filesystem::absolute(mainLuaPath);
		std::filesystem::path corePath = MakeRelativeToExe(coreLuaPath);

		lw.RunScript(missionData); //add mission global object

		if (!lw.RunFile(corePath.string())) return false;
		if (!lw.RunFile(path.string())) return false;

		return lw.RunCsvProducer("extractCsv", std::filesystem::path(csvPath).string());
	}

	bool GetMainLuaOptions(std::vector<std::string>& output) const {

		std::filesystem::path dir = MakeRelativeToExe(mainLuaDir);
		if (!std::filesystem::exists(dir))return false;
		for (const auto& it : std::filesystem::directory_iterator(dir)) {
			const auto& path = it.path();
			if (path.has_filename() && path.extension().string() == ".lua") {
				output.push_back(path.filename().string());
			}
		}
		return true;
	}

	bool CSVExists(const std::string& mizPath, std::filesystem::path& output) const{
		auto inputPath = std::filesystem::path(mizPath);

		output = MakeRelativeToExe(inputCsvDir);

		if (!std::filesystem::exists(output)) std::filesystem::create_directory(output);

		output += inputPath.stem();
		output += ".csv";

		return std::filesystem::exists(output);
	}

	int onError(const std::string& failedAction, const std::string& toLog, int code) const{
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

		LuaWrapper lw(mLogger, mCsv);

		if (!lw.RunFile(path.string())) return false;

		lua_getglobal(lw.L(), "config");

		if (std::map<std::string, std::string> map; lw.ReadFromTable<std::string>(map)) mConfig.ApplyMap(map);

		lua_pop(lw.L(), 1);
		return true;
	}

	bool SaveConfig() {
		std::filesystem::path path = MakeRelativeToExe(configLuaPath);
		std::filesystem::path corePath = MakeRelativeToExe(coreLuaPath);

		LuaWrapper lw(mLogger, mCsv);

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

			if (!csv.good()) return false;
			csv << "-- REMINDER: Use lua escape sequences in string variables!" << std::endl
				<< "--           e.g. [\"" << AppConfig::EditorExePath_key << "\"] = \"C:\\MyApp.exe\"    -- Bad" << std::endl
				<< "--                [\"" << AppConfig::EditorExePath_key << "\"] = \"C:\\\\MyApp.exe\"   -- OK" << std::endl;
			csv << fileContent;
			return true;
		}
		else lua_pop(lw.L(), 2); //core & return value

		return false;
	}

	bool ExecEditorAndWait(const std::filesystem::path &csvPath) const{

		std::string path = "\"" + csvPath.string() + "\"";
		SHELLEXECUTEINFOA shellExecInfo;

		if (mConfig.EditorExePath == "") {//default editor
			
			
			ZeroMemory(&shellExecInfo, sizeof(shellExecInfo));
			shellExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
			shellExecInfo.lpFile = LPCSTR(path.c_str());
			shellExecInfo.nShow = SW_MAXIMIZE;
			shellExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			if (!ShellExecuteExA(&shellExecInfo)) {
				mLogger->Error("ShellExec error code: " + std::to_string(GetLastError()));
				return false;
			}
			if (shellExecInfo.hProcess != nullptr)
			{
				WaitForSingleObject(shellExecInfo.hProcess, INFINITE);
				CloseHandle(shellExecInfo.hProcess);
			}
		}
		else {
			std::string editorPath = "\"" + mConfig.EditorExePath + "\"";
			ZeroMemory(&shellExecInfo, sizeof(shellExecInfo));
			shellExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
			shellExecInfo.lpFile = LPCSTR(editorPath.c_str());
			shellExecInfo.lpParameters = LPCSTR(path.c_str());
			shellExecInfo.nShow = SW_MAXIMIZE;
			shellExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
			if (!ShellExecuteExA(&shellExecInfo)) {
				mLogger->Error("ShellExec error code: " + std::to_string(GetLastError()));
				return false;
			}
			if (shellExecInfo.hProcess != nullptr)
			{
				WaitForSingleObject(shellExecInfo.hProcess, INFINITE);
				CloseHandle(shellExecInfo.hProcess);
			}
		}
		return true;
	}

	std::string GetFromList(const std::vector<std::string>& options, const std::string& prompt = "") {
		if (options.size() == 0)return "";
		else if (options.size() == 1)return options[0];

		if(prompt != "") std::cout << prompt << " Use <Tab> to cycle: ";
		int at = 0;
		std::cout << options[at];
		while (true) {
			int c = _getch();
			if (c == '\t') {
				for (const char &c:options[at]) std::cout << "\b \b";
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
	if(!mc.GetConfig()) return mc.onError("while loading config","",ErrorCodes::LOAD_CONFIG);
	if (!mc.SaveConfig()) return mc.onError("while saving config", "", ErrorCodes::SAVE_CONFIG);

	std::vector<std::string> luaOptions;
	if(!mc.GetMainLuaOptions(luaOptions)||luaOptions.empty())return mc.onError("finding scripts","", ErrorCodes::FIND_SCRIPTS);
	std::string luaFilepath = mc.MakeRelativeToExe(mainLuaDir) .append( mc.GetFromList(luaOptions,"Select script file to run.")).string();


	std::string missionFileContent;

	try {
		if (!mc.Miz()->extractMissionData(missionFilePath, missionFileContent)) return mc.onError("while extracting mission data", "", ErrorCodes::EXTRACTING_MIZ);

		std::filesystem::path csvPath;
		mc.CSVExists(missionFilePath, csvPath);//get csv file path (ignore whether it already exists)

		if (!mc.LuaExtractScript(csvPath,luaFilepath, missionFileContent)) return mc.onError("while generating csv", "", ErrorCodes::SAVING_CSV);
		if (!mc.ExecEditorAndWait(csvPath)) return mc.onError("launching editor", "", ErrorCodes::EDITING_CSV);

		mc.Miz()->backupFile(missionFilePath,mc.MakeRelativeToExe(backupsPath));
		std::string newMissionFileContent;
		if (!mc.LuaApplyScript(csvPath,luaFilepath, missionFileContent, newMissionFileContent)) return mc.onError("while updating mission data", "", ErrorCodes::UPDATING_MIZ);

		if (!mc.Miz()->overwriteMissionData(missionFilePath, newMissionFileContent)) return mc.onError("while saving mission data", "", ErrorCodes::SAVING_MIZ);
	}
	catch (std::exception ex){
		return mc.onError("unexpectedly", ex.what(), ErrorCodes::UNKNOWN);
	}

	return 0;
}