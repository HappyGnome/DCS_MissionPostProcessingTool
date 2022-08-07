#pragma once
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include<memory>
#include<list>
#include<string>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<chrono>

class Logger {
	enum class EventLevel {
		Error,
		Warn,
		Info
	};
	static std::string EventLevelToString(EventLevel l);

	struct LogItem {
		EventLevel mLevel;
		std::string mMessage;
		std::chrono::time_point<std::chrono::system_clock> mWhen;
	
		explicit LogItem(EventLevel level, const std::string& message);
		/*
		LogItem(const LogItem& other);
		LogItem(LogItem&& other) noexcept;
		~LogItem();
		LogItem& operator= (const LogItem& other);
		LogItem& operator= (LogItem&& other) noexcept;*/
	};
	std::list<LogItem> mMessages;
	std::mutex mMessagesMutex;
	std::thread mMessagesHandler;
	bool mKill = false;
	std::condition_variable mMessagesCV;
	std::string mFilePath;

	void Work();
	void Log(EventLevel level, const std::string& message);
public: 
	explicit Logger(const std::string& filePath);
	~Logger();

	void Error(const std::string& message);
	void Warn(const std::string& message);
	void Info(const std::string& message);
};

class LoggerSection {
	std::shared_ptr<Logger> mLogger;
	std::string mPrefix;
public:
	explicit LoggerSection(std::shared_ptr<Logger> parent, const std::string& prefix);

	void Error(const std::string& message) const;
	void Warn(const std::string& message) const;
	void Info(const std::string& message) const;
};

#endif


