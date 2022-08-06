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

class LuaWrapper {
	lua_State* mL;
public:
	bool ExtractCsvContent(const std::string& filePath);


	LuaWrapper();
	~LuaWrapper();
	lua_State* L();

	bool RunFile(const std::string& filePath);
	bool RunScript(const std::string& script);
	bool CsvFileToLua(const std::string& filePath);

	bool RunCsvProducer(const std::string& functionName, const std::string& outputPath);

	bool RunCsvHandler(const std::string& functionName, const std::string& csvPath, std::string& output);
	//bool GlobalToString(const std::string& global, std::string& output);
};

#endif