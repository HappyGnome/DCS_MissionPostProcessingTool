#include "LuaCsvHelper.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <fstream>
#include <string>
#include <filesystem>
#include "CsvFileTools.h"
#include "StringHelpers.h"

LuaCsvHelper::LuaCsvHelper(std::shared_ptr<Logger> logger, std::shared_ptr<CsvFileTools> csvt):LuaWrapper(logger), mCsvTools(csvt) {}
LuaCsvHelper::~LuaCsvHelper() {}

bool LuaCsvHelper::RunCsvProducer(const std::string& functionName, const std::string& outputPath) {
	lua_getfield(mL, LUA_GLOBALSINDEX, functionName.c_str());	
	if (!lua_isfunction(mL, -1)) {
		lua_pop(mL, 1);
		return false;
	}
	if (lua_pcall(mL, 0, 1, 0))
	{
		LogLuaError();
		return false;
	}
	return ExtractCsvContent(outputPath);
}

//Call named global function accepting 2D table (csv data from a file), returning a string
bool LuaCsvHelper::RunCsvHandler(const std::string& functionName, const std::string& csvPath, std::string& output) {
	if (!GetGlobalFunction(functionName)) return false;
	if (!CsvFileToLua(csvPath))
	{
		lua_pop(mL, 1);
		return false;
	}
	if (!CallFunction(1,1)) return false;

	if (!lua_isstring(mL, -1)) {
		lua_pop(mL, 1);
		return false;
	}

	output = lua_tostring(mL, -1);
	lua_pop(mL, 1);

	return true;
}

//pop 2D array from Lua and output to a csv file, overwriting existing content
bool LuaCsvHelper::ExtractCsvContent(const std::string& filePath) {

	std::string output = "";

	if (!lua_istable(mL, -1)) return false;

	lua_pushnil(mL);
	while (lua_next(mL, -2) != 0 && lua_istable(mL, -1)) { //iterate rows
		bool newLine = true;
		lua_pushnil(mL);
		while (lua_next(mL, -2) != 0) {//iterate columns
			if (!newLine) output += ",";			
			
			if (int type = lua_type(mL, -1); type == LUA_TSTRING) output += mCsvTools->EscapeCSVEntry(lua_tostring(mL, -1),true);
			else if (type == LUA_TNUMBER) output += std::to_string(lua_tonumber(mL, -1));
			else if (type == LUA_TBOOLEAN) output += str_helpers::BToS(lua_toboolean(mL, -1));
			else if (type == LUA_TNIL) output += str_helpers::NilToS();

			lua_pop(mL, 1);//pop value
			newLine = false;
		}
		output += "\n";
		lua_pop(mL, 1);//pop value (row)
	}

	std::ofstream csv(filePath, std::ios_base::trunc);

	if (!csv.good()) return false;
	csv << output;

	return true;
}

bool LuaCsvHelper::CsvFileToLua(const std::string& filePath) {

	std::ifstream csv(filePath);

	if (!csv.good()) return false;

	lua_newtable(mL);

	int i = 1;
	while (!csv.eof()) {
		std::string line;
		std::getline(csv, line);
		if (line == "") continue;
		std::vector<CsvEntry> toks = mCsvTools->SplitCSVLine(line);

		lua_newtable(mL);

		int k = 1;
		for (const CsvEntry& s : toks)
		{
			const char* str = s.str.c_str();

			if (s.literal) lua_pushstring(mL, str);
			else if (str_helpers::TryAToNil(str)) lua_pushnil(mL);
			else if (int val; str_helpers::TryAToI(str, val)) lua_pushinteger(mL,val);
			else if (float f; str_helpers::TryAToF(str, f)) lua_pushnumber(mL, f);
			else if (int b; str_helpers::TryAToB(str,b))lua_pushboolean(mL, b);
			else lua_pushstring(mL, str);

			lua_rawseti(mL, -2, k);
			k++;
		}
		lua_rawseti(mL, -2, i);
		i++;
	}
	return true;
}