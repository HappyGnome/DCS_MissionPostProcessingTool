#include "Logger.h"
#include<fstream>
#include<filesystem>
#include<time.h>

std::string Logger::EventLevelToString(EventLevel l){
	switch (l) {
		case EventLevel::Error: return "ERROR";
		case EventLevel::Warn: return "WARN";
		case EventLevel::Info: return "INFO";
		default: return "??";
	}
}

Logger::LogItem::LogItem(EventLevel level, const std::string& message) :mLevel(level), mMessage(message) {
	mWhen = std::chrono::system_clock::now();
}
/*
Logger::LogItem::LogItem(const LogItem& other) = default;
Logger::LogItem::LogItem(LogItem&& other) noexcept:
											mLevel(std::exchange(other.mLevel, nullptr)),
											mMessage(std::exchange(other.mMessage, nullptr)),
											mWhen(std::exchange(other.mWhen, nullptr)) {}

Logger::LogItem::~LogItem() = default;
Logger::LogItem& Logger::LogItem::operator= (const LogItem& other) {
	return *this = LogItem(other);
}
Logger::LogItem& Logger::LogItem::operator= (LogItem&& other) noexcept {
	std::swap(mLevel, other.mLevel);
	std::swap(mMessage, other.mMessage);
	std::swap(mWhen, other.mWhen);
	return *this;
}*/

//-----------------------------------------------------------------------------------------------------------------------
void Logger::Work() {
	std::ofstream file(mFilePath, std::ios_base::trunc);
	if (!file.good()) return;

	Info("Logging thread started");
	while (true) {
		std::list<LogItem> messages;
		{
			std::unique_lock lg(mMessagesMutex);
			mMessagesCV.wait(lg, [this] {return !mMessages.empty() || mKill; });
			std::swap(messages, mMessages);
		}
		
		for (const LogItem& li : messages) {			
			auto minutes = std::chrono::time_point_cast<std::chrono::minutes>(li.mWhen);
			auto mils = std::chrono::duration_cast<std::chrono::milliseconds>(li.mWhen - minutes);

			tm timeBuf;
			std::time_t baseTime = std::chrono::system_clock::to_time_t(minutes);
			gmtime_s(&timeBuf, &baseTime);

			file << std::put_time(&timeBuf, "%F %H:%M:")<<std::setprecision(3) << (float)mils.count()/1000.0f << "|" << Logger::EventLevelToString(li.mLevel) << "|" << li.mMessage << std::endl;
		}
		if (mKill) break;
	}
	file.flush();
	file.close();
}

Logger::Logger(const std::string& filePath){
	mFilePath = std::filesystem::absolute(filePath).string();
	mMessagesHandler = std::thread(&Logger::Work,this);
}

Logger::~Logger() {
	{
		std::unique_lock lg(mMessagesMutex);
		mKill = true;
	}
	mMessagesCV.notify_one();
	mMessagesHandler.join();
}

void Logger::Log(EventLevel level, const std::string& message) {
	std::unique_lock lg(mMessagesMutex);

	mMessages.emplace_back(level, message);
	mMessagesCV.notify_one();
}

void Logger::Error(const std::string& message) {
	Log(EventLevel::Error, message);
}
void Logger::Warn(const std::string& message) {
	Log(EventLevel::Warn, message);
}
void Logger::Info(const std::string& message) {
	Log(EventLevel::Info, message);
}

//----------------------------------------------------------------------------------------------
LoggerSection::LoggerSection(std::shared_ptr<Logger> parent, const std::string& prefix): mLogger(parent),mPrefix(prefix){}

void LoggerSection::Error(const std::string& message) const{
	mLogger->Error(mPrefix + "|" + message);
}
void LoggerSection::Warn(const std::string& message) const {
	mLogger->Warn(mPrefix + "|" + message);
}
void LoggerSection::Info(const std::string& message) const {
	mLogger->Info(mPrefix + "|" + message);
}