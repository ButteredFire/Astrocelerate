#include "LoggingManager.hpp"
#include <Engine/Threading/ThreadManager.hpp>


namespace LogSpacing {
#if defined(_WIN32)
	const int THREAD_INFO_MAX_WIDTH_OS = 28;
#elif defined(__linux__)
	const int THREAD_INFO_MAX_WIDTH_OS = 30;
#elif defined(__APPLE__)
	const int THREAD_INFO_MAX_WIDTH_OS = 40;
#else
	const int THREAD_INFO_MAX_WIDTH_OS = 50;
#endif

	const int DISPLAY_TYPE_WIDTH = 9;
	const int CALLER_WIDTH = 40;
}


namespace Log {
	std::deque<LogMessage> LogBuffer;
	int MaxLogLines = 1000;


	void LogThreadInfo(std::string &output) {
		std::thread::id currentThread = std::this_thread::get_id();
		std::stringstream ss;
		ss << "[";
		if (currentThread == ThreadManager::GetMainThreadID())
			ss << "MAIN]";
		else
			ss << "WORKER]";

		ss << "[THREAD " << ThreadManager::ThreadIDToString(currentThread) << "] ";

		output = ss.str();
	}


	void Print(MsgType type, const char *caller, const std::string &message, bool newline) {
		std::lock_guard<std::mutex> lock(_printMutex);

		// Get message type
		std::string displayType = "Unknown message type";
		LogColor(type, displayType);

		// Get thread info
		std::string threadInfo = "";
		LogThreadInfo(threadInfo);

		// Log
		std::stringstream ss;
		ss << std::left << std::setw(LogSpacing::THREAD_INFO_MAX_WIDTH_OS) << threadInfo
			<< std::left << std::setw(LogSpacing::DISPLAY_TYPE_WIDTH) << ("[" + displayType + "]")
			<< "[ " << std::left << std::setw(LogSpacing::CALLER_WIDTH) << caller << "]: "
			<< message << ((newline) ? "\n" : "");

		if (type == MsgType::T_ERROR || type == MsgType::T_FATAL)
			std::cerr << ss.str() << termcolor::reset;
		else
			std::cout << ss.str() << termcolor::reset;

			// Write to file
		if (_logFile.is_open()) {
			_logFile << ss.str();
			_logFile.flush();
		}


		// Push to log buffer
		LogMessage msg{};
		msg.type = type;
		msg.displayType = displayType;
		msg.threadInfo = threadInfo;
		msg.caller = caller;
		msg.message = ss.str();

		AddToLogBuffer(msg);
	}


	RuntimeException::RuntimeException(const std::string &functionName, const int errLine, const std::string &message, MsgType severity) : m_funcName(functionName), m_errLine(errLine), m_exceptionMessage(message), m_msgType(severity) {

		std::thread::id currentThread = std::this_thread::get_id();
		std::string threadName = " " + STD_STR((currentThread == ThreadManager::GetMainThreadID()) ? "(Main)" : "(Worker)");
		m_threadInfo = ThreadManager::ThreadIDToString(currentThread) + threadName;

		std::string displayType = "Unknown message type";
		LogColor(m_msgType, displayType);

		std::string threadInfo = "";
		LogThreadInfo(threadInfo);

		LogMessage msg{};
		msg.type = m_msgType;
		msg.displayType = displayType;
		msg.threadInfo = threadInfo;
		msg.caller = m_funcName;
		msg.message = m_exceptionMessage;

		AddToLogBuffer(msg);
	}
}