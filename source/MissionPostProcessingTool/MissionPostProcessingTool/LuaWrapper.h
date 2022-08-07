#pragma once
#ifndef __LUA_WRAPPER__
#define __LUA_WRAPPER__

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <fstream>
#include <string>
#include <filesystem>
#include "Logger.h"
#include "CsvFileTools.h"

class LuaWrapper {
	lua_State* mL;
	LoggerSection mLogger;
	std::shared_ptr<CsvFileTools> mCsvTools;

	void LogLuaError();
public:
	bool ExtractCsvContent(const std::string& filePath);


	explicit LuaWrapper(std::shared_ptr<Logger> logger, std::shared_ptr<CsvFileTools> csvt);
	~LuaWrapper();
	lua_State* L();

	bool RunFile(const std::string& filePath);
	bool RunScript(const std::string& script);
	bool CsvFileToLua(const std::string& filePath);

	bool RunCsvProducer(const std::string& functionName, const std::string& outputPath);

	bool RunCsvHandler(const std::string& functionName, const std::string& csvPath, std::string& output);
};

#endif