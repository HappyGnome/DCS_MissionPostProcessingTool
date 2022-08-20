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
#include<map>
#include "Logger.h"

class LuaWrapper {
protected:
	lua_State* mL;
	LoggerSection mLogger;

	void LogLuaError();
public:


	explicit LuaWrapper(std::shared_ptr<Logger> logger);
	~LuaWrapper();
	lua_State* L();

	bool RunFile(const std::string& filePath);
	bool RunScript(const std::string& script);
	bool GetGlobalFunction(const std::string& functionName);
	bool CallFunction(int argc,int retc);

	template<typename T>
	bool ReadFromTable(std::map<std::string, T, std::less<std::string>>& output) { return false; }

	template<>
	bool ReadFromTable<std::string>(std::map<std::string, std::string, std::less<std::string>>& output) {
		if (!lua_istable(mL, -1)) return false;

		lua_pushnil(mL);
		while (lua_next(mL, -2) != 0) {//iterate table
			if (int type = lua_type(mL, -1); type == LUA_TSTRING) output.insert_or_assign(lua_tostring(mL, -2), lua_tostring(mL, -1));
			lua_pop(mL, 1);//pop value
		}
		return true;
	}

	template<>
	bool ReadFromTable<float>(std::map<std::string, float,std::less<std::string>>& output) {
		if (!lua_istable(mL, -1)) return false;

		lua_pushnil(mL);
		while (lua_next(mL, -2) != 0) {//iterate table
			if (int type = lua_type(mL, -1); type == LUA_TNUMBER) output.insert_or_assign(lua_tostring(mL, -2), lua_tonumber(mL, -1));
			lua_pop(mL, 1);//pop value
		}
		return true;
	}

	template<>
	bool ReadFromTable<bool>(std::map<std::string, bool, std::less<std::string>>& output) {
		if (!lua_istable(mL, -1)) return false;

		lua_pushnil(mL);
		while (lua_next(mL, -2) != 0) {//iterate table
			if (int type = lua_type(mL, -1); type == LUA_TBOOLEAN) output.insert_or_assign(lua_tostring(mL, -2), lua_toboolean(mL, -1));
			lua_pop(mL, 1);//pop value
		}
		return true;
	}

	//Add dictionary items to table at top of lua stack
	template<typename T>
	bool WriteToTable(const std::map<std::string, T, std::less<std::string>>& map) { return false; }

	template<>
	bool WriteToTable<std::string>(const std::map<std::string, std::string, std::less<std::string>>& map) { 
		if (!lua_istable(mL, -1))return false;

		for (auto it = map.cbegin(); it != map.cend();++it) {
			lua_pushstring(mL,it->second.c_str());
			lua_setfield(mL, -2, it->first.c_str());
		}
		return true;
	}

	template<>
	bool WriteToTable<float>(const std::map<std::string, float, std::less<std::string>>& map) {
		if (!lua_istable(mL, -1))return false;

		for (auto it = map.cbegin(); it != map.cend(); ++it) {
			lua_pushnumber(mL, it->second);
			lua_setfield(mL, -2, it->first.c_str());
		}
		return true;
	}
	template<>
	bool WriteToTable<bool>(const std::map<std::string, bool, std::less<std::string>>& map) {
		if (!lua_istable(mL, -1))return false;

		for (auto it = map.cbegin(); it != map.cend(); ++it) {
			lua_pushboolean(mL, it->second);
			lua_setfield(mL, -2, it->first.c_str());
		}
		return true;
	}
};

#endif