#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

//#include<windows.h>
//#include<processthreadsapi.h>
#include<shlwapi.h>
#include<shlobj.h>

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

constexpr const char* mainLuaPath = "scripts\\main.lua";
constexpr const char* coreLuaPath = "scripts\\core\\core.lua";
constexpr const char* inputCsvDir = "\\input\\";
constexpr const char* configLuaPath = "config.lua";

class MainContext
{
	std::shared_ptr<Logger> mLogger = std::make_shared<Logger>("MissionPostProcessingTool.log");
	std::shared_ptr <MizFileTools> mMiz = std::make_shared<MizFileTools>(mLogger);
	std::shared_ptr <CsvFileTools> mCsv = std::make_shared<CsvFileTools>(mLogger);

	AppConfig mConfig;
public:	

	std::shared_ptr <MizFileTools> Miz() const { return mMiz; }
	std::shared_ptr <CsvFileTools> Csv() const { return mCsv; }

	bool LuaApplyScript(const std::filesystem::path& csvPath, const std::string& missionData, std::string& newMissionData) const{

		LuaWrapper lw(mLogger,mCsv);

		std::filesystem::path path = std::filesystem::absolute(mainLuaPath);
		std::filesystem::path corePath = std::filesystem::absolute(coreLuaPath);

		lw.RunScript(missionData);//add mission global object

		if (!lw.RunFile(corePath.string())) return false;
		if (!lw.RunFile(path.string())) return false;

		return lw.RunCsvHandler("applyCsv", std::filesystem::absolute(csvPath).string(), newMissionData);
	}

	bool LuaExtractScript(const std::filesystem::path& csvPath, const std::string& missionData) const{

		LuaWrapper lw(mLogger,mCsv);

		std::filesystem::path path = std::filesystem::absolute(mainLuaPath);
		std::filesystem::path corePath = std::filesystem::absolute(coreLuaPath);

		lw.RunScript(missionData); //add mission global object

		if (!lw.RunFile(corePath.string())) return false;
		if (!lw.RunFile(path.string())) return false;

		return lw.RunCsvProducer("extractCsv", std::filesystem::absolute(csvPath).string());
	}

	bool CSVExists(const std::string& mizPath, std::filesystem::path& output) const{
		auto inputPath = std::filesystem::path(mizPath);

		output = std::filesystem::current_path();
		output += inputCsvDir;

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
		std::filesystem::path path = std::filesystem::absolute(configLuaPath);
		if (!std::filesystem::exists(path)) return true;//nothing to load

		LuaWrapper lw(mLogger, mCsv);

		if (!lw.RunFile(path.string())) return false;

		lua_getglobal(lw.L(), "config");

		if (std::map<std::string, std::string> map; lw.ReadFromTable<std::string>(map)) mConfig.ApplyMap(map);

		lua_pop(lw.L(), 1);
		return true;
	}

	bool SaveConfig() {
		std::filesystem::path path = std::filesystem::absolute(configLuaPath);
		std::filesystem::path corePath = std::filesystem::absolute(coreLuaPath);

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

	bool ExecEditorAndWait(std::filesystem::path csvPath) {

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
};

int main(int argc, char* argv[]) {

	if (argc < 2)return 0;
	auto missionFilePath = std::string(argv[1]);

	MainContext mc;	
	if(!mc.GetConfig()) return mc.onError("while loading config","",-5);
	if (!mc.SaveConfig()) return mc.onError("while saving config", "", -6);

	std::string missionFileContent;

	try {
		if (!mc.Miz()->extractMissionData(missionFilePath, missionFileContent)) return mc.onError("while extracting mission data", "", -1);

		std::filesystem::path csvPath;
		mc.CSVExists(missionFilePath, csvPath);//get csv file path (ignore whether it already exists)

		if (!mc.LuaExtractScript(csvPath, missionFileContent)) return mc.onError("while generating csv", "", -2);
		if (!mc.ExecEditorAndWait(csvPath)) return mc.onError("launching editor", "", -7);

		mc.Miz()->backupFile(missionFilePath);
		std::string newMissionFileContent;
		if (!mc.LuaApplyScript(csvPath, missionFileContent, newMissionFileContent)) return mc.onError("while updating mission data", "", -3);

		if (!mc.Miz()->overwriteMissionData(missionFilePath, newMissionFileContent)) return mc.onError("while saving mission data", "", -4);
	}
	catch (std::exception ex){
		return mc.onError("unexpectedly", ex.what(), -4);
	}

	return 0;
}