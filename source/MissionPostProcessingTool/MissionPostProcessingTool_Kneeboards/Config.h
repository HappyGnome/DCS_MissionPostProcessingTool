#pragma once
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include<string>
#include<map>

struct AppConfig {

	//ApplyMap -----------------------------------------------------
	template<typename T>
	void ApplyMap(const std::map<std::string, T, std::less<std::string>> &map) {}

	template<>
	void ApplyMap<std::string>(const std::map<std::string, std::string, std::less<std::string>>& map) {
	}
	//AddToMap -----------------------------------------------------
	template<typename T>
	void AddToMap(std::map<std::string, T, std::less<std::string>>& output) {}

	template<>
	void AddToMap<std::string>(std::map<std::string, std::string, std::less<std::string>>& output) {
	}
};

#endif