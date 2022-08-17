#pragma once
#ifndef __LUACSVHELPER_H__
#define __LUACSVHELPER_H__

#include"LuaWrapper.h"
#include"CsvFileTools.h"

class LuaCsvHelper:public LuaWrapper {
private:
	std::shared_ptr<CsvFileTools> mCsvTools;
public:
	bool ExtractCsvContent(const std::string& filePath);


	explicit LuaCsvHelper(std::shared_ptr<Logger> logger, std::shared_ptr<CsvFileTools> csvt);
	~LuaCsvHelper();


	bool CsvFileToLua(const std::string& filePath);

	bool RunCsvProducer(const std::string& functionName, const std::string& outputPath);

	bool RunCsvHandler(const std::string& functionName, const std::string& csvPath, std::string& output);
};

#endif