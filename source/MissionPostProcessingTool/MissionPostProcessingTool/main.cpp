#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

extern "C" {
	#include <zip.h>
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

#include "main.h"

#include "CsvFileTools.h"
#include "MizFileTools.h"
#include "LuaWrapper.h"
#include "Logger.h"

constexpr const char* mainLuaPath = "scripts\\main.lua";
constexpr const char* coreLuaPath = "scripts\\core\\core.lua";
constexpr const char* inputCsvDir = "\\input\\";

class MainContext
{
	std::shared_ptr<Logger> mLogger = std::make_shared<Logger>("MissionPostProcessingTool.log");
	std::shared_ptr <MizFileTools> mMiz = std::make_shared<MizFileTools>(mLogger);
	std::shared_ptr <CsvFileTools> mCsv = std::make_shared<CsvFileTools>(mLogger);
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
};

int main(int argc, char* argv[]) {

	if (argc < 2)return 0;
	auto missionFilePath = std::string(argv[1]);

	MainContext mc;	

	std::string missionFileContent;

	try {
		if (!mc.Miz()->extractMissionData(missionFilePath, missionFileContent)) return mc.onError("while extracting mission data", "", -1);

		if (std::filesystem::path csvPath; !mc.CSVExists(missionFilePath, csvPath)) 
			if(!mc.LuaExtractScript(csvPath, missionFileContent)) return mc.onError("while generating csv", "", -2);
		else {

			mc.Miz()->backupFile(missionFilePath);
			std::string newMissionFileContent;
			if (!mc.LuaApplyScript(csvPath, missionFileContent, newMissionFileContent)) return mc.onError("while updating mission data", "", -3);

			if (!mc.Miz()->overwriteMissionData(missionFilePath, newMissionFileContent)) return mc.onError("while saving mission data", "", -4);
		}
	}
	catch (std::exception ex){
		return mc.onError("unexpectedly", ex.what(), -4);
	}

	return 0;
}