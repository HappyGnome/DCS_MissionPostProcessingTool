#include "LuaWrapper.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <fstream>
#include <string>
#include <filesystem>
#include "StringHelpers.h"

LuaWrapper::LuaWrapper(std::shared_ptr<Logger> logger):mLogger(logger,"LUA") {
	mL = lua_open();

	luaopen_base(mL);
	luaopen_table(mL);
	luaopen_string(mL);
	luaopen_math(mL);
}
LuaWrapper::~LuaWrapper() {
	lua_close(mL);
}
lua_State* LuaWrapper::L() {
	return mL;
}

void LuaWrapper::LogLuaError() {
	if (lua_isstring(mL, -1)) {
		const char* msg = lua_tostring(mL, -1);
		lua_pop(mL, 1);
		mLogger.Error(msg);
	}
}

bool LuaWrapper::RunFile(const std::string& path) {
	if (!luaL_dofile(mL, path.c_str())) return true;
	else {
		LogLuaError();
		return false;
	}
}

bool LuaWrapper::RunScript(const std::string& script) {
	if (!luaL_dostring(mL, script.c_str()))return true;
	else {
		LogLuaError();
		return false;
	}
}

//Call named global function accepting 2D table (csv data from a file), returning a string
bool LuaWrapper::GetGlobalFunction(const std::string& functionName) {
	lua_getfield(mL, LUA_GLOBALSINDEX, functionName.c_str());
	if (!lua_isfunction(mL, -1)) {
		lua_pop(mL, 1);
		return false;
	}
	return true;
}
bool  LuaWrapper::CallFunction(int argc, int retc) {
	if (lua_pcall(mL, argc,retc, 0)) {
		LogLuaError();
		return false;
	}
	return true;
}