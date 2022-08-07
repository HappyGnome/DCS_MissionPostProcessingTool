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

//-----------------------------------------------------------------------------------------------------------------------
void Logger::Work() {
	std::ofstream file(mFilePath, std::ios_base::trunc);
	if (!file.good()) return;

	bool kill = false;
	Info("Logging thread started");
	while (!kill) {
		std::list<LogItem> messages;
		{
			std::unique_lock lg(mMessagesMutex);
			mMessagesCV.wait(lg, [this] {return !mMessages.empty() || mKill; });
			std::swap(messages, mMessages);
			kill = mKill;
		}
		
		for (const LogItem& li : messages) {			
			auto minutes = std::chrono::time_point_cast<std::chrono::minutes>(li.mWhen);
			auto mils = std::chrono::duration_cast<std::chrono::milliseconds>(li.mWhen - minutes);

			tm timeBuf;
			std::time_t baseTime = std::chrono::system_clock::to_time_t(minutes);
			gmtime_s(&timeBuf, &baseTime);

			file << std::put_time(&timeBuf, "%F %H:%M:")<<std::setprecision(5) << (float)mils.count()/1000.0f << "\t|" << Logger::EventLevelToString(li.mLevel) << "\t|" << li.mMessage << std::endl;
		}
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