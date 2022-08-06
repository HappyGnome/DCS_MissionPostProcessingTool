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

constexpr const char* mainLuaPath = "scripts\\main.lua";
constexpr const char* coreLuaPath = "scripts\\core\\core.lua";
constexpr const char* inputCsvDir = "\\input\\";


bool LuaApplyScript(const std::filesystem::path& csvPath, const std::string& missionData, std::string& newMissionData){

	LuaWrapper lw;

	std::filesystem::path path = std::filesystem::absolute(mainLuaPath);
	std::filesystem::path corePath = std::filesystem::absolute(coreLuaPath);

	lw.RunScript(missionData);//add mission global object

	if (!lw.RunFile(corePath.string())) return false;
	if(!lw.RunFile(path.string())) return false;

	return lw.RunCsvHandler("applyCsv", std::filesystem::absolute(csvPath).string(), newMissionData);
}

bool LuaExtractScript(const std::filesystem::path& csvPath, const std::string& missionData) {

	LuaWrapper lw;

	std::filesystem::path path = std::filesystem::absolute(mainLuaPath);
	std::filesystem::path corePath = std::filesystem::absolute(coreLuaPath);

	lw.RunScript(missionData); //add mission global object

	if (!lw.RunFile(corePath.string())) return false;
	if (!lw.RunFile(path.string())) return false;

	return lw.RunCsvProducer("extractCsv", std::filesystem::absolute(csvPath).string());
}

bool CSVExists(const std::string &mizPath, std::filesystem::path &output) {
	auto inputPath = std::filesystem::path(mizPath);

	output = std::filesystem::current_path();
	output += inputCsvDir;

	if (!std::filesystem::exists(output)) std::filesystem::create_directory(output);

	output += inputPath.stem();
	output += ".csv";

	return std::filesystem::exists(output);
}

int main(int argc, char* argv[]) {

	if (argc < 2)return 0;

	auto missionFilePath = std::string(argv[1]);

	std::string missionFileContent;

	if (!MizFileTools::extractMissionData(missionFilePath, missionFileContent))return -1;

	if (std::filesystem::path csvPath; !CSVExists(missionFilePath, csvPath)) LuaExtractScript(csvPath, missionFileContent);
	else {

		MizFileTools::backupFile(missionFilePath);
		std::string newMissionFileContent;
		if(!LuaApplyScript(csvPath, missionFileContent, newMissionFileContent)) return -3;

		if (!MizFileTools::overwriteMissionData(missionFilePath, newMissionFileContent))return -2;
	}

	return 0;
}