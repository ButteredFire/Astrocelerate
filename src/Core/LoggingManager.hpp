#pragma once

#include <iostream>
#include <string>
#include <exception>
#include <deque>
#include <termcolor/termcolor.hpp>

#include <Core/Constants.h>

#define enquote(S) std::string(std::string("\"") + std::string(S) + std::string("\""))
#define enquoteCOUT(S) '"' << (S) << '"'
#define VARIABLE_NAME(V) std::string((#V))
#define BOOLALPHA(COND) ((COND) ? "true" : "false")
#define BOOLALPHACAP(COND) ((COND) ? "True" : "False")

// Usage: LOG_ASSERT(condition, message [, severity])
#define LOG_ASSERT(cond, msg, ...) \
    do { \
        if (!(cond)) { \
            throw Log::RuntimeException(__FUNCTION__, __LINE__, std::string(msg), ##__VA_ARGS__); \
        } \
    } while (0)

namespace Log {
	/* Purpose of each message type:
	* - INFO: Used to log general information about the application's operation. This includes high-level events that are part of the normal operation.
	*		Examples: Service initialization/stopping, user actions, configuration changes
	* 
	* - VERBOSE: Used to log very detailed logging that helps with tracing the exact flow of the application.
	*		Examples: function calls (begin/end), initialization details, data parsing
	* 
	* - DEBUG: Used to log very detailed logging that helps with understanding the application's current state and behavior.
	*		Examples: Variable states, function call status, microservice execution status
	* 
	* - WARNING: Used to log potentially harmful situations which are not necessarily errors but could lead to problems.
	*		Examples: Low disk space, configuration issues, missing optional/non-critical data
	* 
	* - ERROR: Used to log errors that are fatal to the operation, and not the service/application. Such errors may terminate the application, but do not indicate any serious eventuality, such as data corruption.
	*		Examples: Failure to open file, missing critical data
	* 
	* - FATAL: Used to log severe errors that necessitate service/application termination to prevent (further) data loss or corruption.
	*		Examples: Failure to completely record data, failure to completely modify data
	* 
	* - SUCCESS: Used to log successful operations.
	*		Examples: Successful task completion, successful data processing
	*/
	enum MsgType {
		T_VERBOSE,
		T_DEBUG,
		T_INFO,
		T_WARNING,
		T_ERROR,
		T_FATAL,
		T_SUCCESS
	};


	struct LogMessage {
		MsgType type;				// The message type.
		std::string displayType;	// The message type in the form of a string.
		std::string caller;			// The origin of the message.
		std::string message;		// The message content.
	};


	// The log buffer (used for logging to the GUI console)
	extern std::deque<LogMessage> LogBuffer;
	static constexpr size_t MAX_LOG_LINES = 500;


	/* Adds a message to the log buffer.
		@param logMsg: The message to be added to the log buffer.
	*/
	inline void AddToLogBuffer(LogMessage& logMsg) {
		LogBuffer.push_back(logMsg);
		if (LogBuffer.size() > MAX_LOG_LINES)
			LogBuffer.pop_front();
	}


	/* Outputs a color to the output stream based on message type.
		Optionally, set outputColor to False to get the message type (as a string).
	*/
	inline void LogColor(MsgType type, std::string& msgType, bool outputColor = true) {
		switch (type) {
			case T_VERBOSE:
				msgType = "VERBOSE";
				if (outputColor) std::cout << termcolor::bright_grey;
				break;

			case T_DEBUG:
				msgType = "DEBUG";
				if (outputColor) std::cout << termcolor::bright_grey;
				break;

			case T_INFO:
				msgType = "INFO";
				if (outputColor) std::cout << termcolor::white;
				break;

			case T_WARNING:
				msgType = "WARNING";
				if (outputColor) std::cout << termcolor::yellow;
				break;

			case T_ERROR:
				msgType = "ERROR";
				if (outputColor) std::cout << termcolor::red;
				break;

			case T_FATAL:
				msgType = "FATAL";
				if (outputColor) std::cout << termcolor::white << termcolor::on_red;
				break;

			case T_SUCCESS:
				msgType = "SUCCESS";
				std::cout << termcolor::bright_green;
				break;

			default: break;
		}
	}

	/* Logs a message. 
		@param type: The message type. Refer to Log::MsgType to see supported types.
		@param caller: The name of the function from which this function is called. It should always be __FUNCTION__.
		@param message: The message to be logged.
		@param newline (default: true): A boolean determining whether the message ends with a newline character (true), or not (false).
	*/
	inline void Print(MsgType type, const char* caller, const std::string& message, bool newline = true) {
		std::string displayType = "Unknown message type";
		
		LogColor(type, displayType);

		std::cout << "[" << displayType << " @ " << caller << "]: " << message << ((newline) ? "\n" : "") << termcolor::reset;

		LogMessage msg{};
		msg.type = type;
		msg.displayType = displayType;
		msg.caller = caller;
		msg.message = message;

		AddToLogBuffer(msg);
	}


	/* Logs application information to the console. */
	inline void PrintAppInfo() {
		std::cout << "Project " << APP_NAME << " (version: " << APP_VERSION << ").\n";
		std::cout << "Project is run in ";
		if (inDebugMode)
			std::cout << "Debug mode.\n\n";
		else
			std::cout << "Release mode.\n\n";

		std::cout << "Compiler information:\n";

		#if defined(_MSC_VER)
			std::cout << "\t- Compiler: Microsoft Visual C++ (MSVC)\n";
			std::cout << "\t- Version: " << _MSC_VER << "\n";
			// _MSC_FULL_VER provides a more detailed version
			std::cout << "\t- Full Version: " << _MSC_FULL_VER << "\n";

			#if _MSVC_LANG   // _MSVC_LANG indicates the C++ standard version (e.g., 201703L for C++17)
				std::cout << "\t- C++ Standard: " << _MSVC_LANG << "L\n";
			#endif
		#elif defined(__clang__)
			std::cout << "\t- Compiler: Clang\n";
			std::cout << "\t- Version: " << __clang_major__ << "." << __clang_minor__ << "." << __clang_patchlevel__ << "\n";
		#elif defined(__GNUC__) || defined(__GNUG__)
			std::cout << "\t- Compiler: GCC (GNU Compiler Collection)\n";
			std::cout << "\t- Version: " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__ << "\n";
		#else
			std::cout << "\t- Compiler: Unknown\n";
		#endif

		std::cout << "\nCopyright (c) 2024-2025 " << AUTHOR << ".\n\n";
	}
	

	/* Custom RTE exception class that allows for origin specification. */
	class RuntimeException : public std::exception {
	public:
		RuntimeException(const std::string& functionName, const int errLine, const std::string& message, MsgType severity = T_ERROR) : m_funcName(functionName), errLine(errLine), m_exceptionMessage(message), m_msgType(severity) {
		
			std::string displayType = "Unknown message type";
			LogColor(m_msgType, displayType);

			LogMessage msg{};
			msg.type = m_msgType;
			msg.displayType = displayType;
			msg.caller = m_funcName;
			msg.message = m_exceptionMessage;

			AddToLogBuffer(msg);
		}

		/* Gets the name of the origin from which the exception was raised. 
			@return The name of the origin.
		*/
		inline const char* origin() const noexcept {
			return m_funcName.c_str();
		}

		/* Gets the source code line on which the exception was thrown.
			@return The line.
		*/
		inline const int errorLine() const noexcept {
			return errLine;
		}

		/* Gets the message's severity.
			@return The message severity.
		*/
		inline const MsgType severity() const noexcept {
			return m_msgType;
		}

		/* Gets the error message. 
			@return The error message.
		*/
		inline const char* what() const noexcept override {
			return m_exceptionMessage.c_str();
		}
	
	private:
		std::string m_funcName = "unknown origin";
		int errLine;
		std::string m_exceptionMessage;
		MsgType m_msgType;
	};
}