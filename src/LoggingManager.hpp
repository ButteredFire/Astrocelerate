#pragma once

#include <iostream>
#include <string>
#include <exception>
#include <termcolor/termcolor.hpp>

#define enquote(S) std::string(std::string("\"") + S + std::string("\""))
#define enquoteCOUT(S) '"' << (S) << '"'
#define VARIABLE_NAME(V) (#V)

namespace Log {
	enum MsgType {
		T_VERBOSE,
		T_INFO,
		T_WARNING,
		T_ERROR
	};

	/* Logs a message. 
	* @param type: The message type. Refer to Log::MsgType to see supported types.
	* @param caller: The name of the function from which this function is called. It should always be __FUNCTION__.
	* @param message: The message to be logged.
	* @param newline (default: true): A boolean determining whether the message ends with a newline character (true), or not (false).
	*/
	static void print(MsgType type, const char* caller, const std::string& message, bool newline = true) {
		std::string msgType = "Unknown message type";
		switch (type) {
		case T_VERBOSE:
			msgType = "VERBOSE";
			std::cout << termcolor::bright_cyan;
			break;

		case T_INFO:
			msgType = "INFO";
			std::cout << termcolor::white;
			break;

		case T_WARNING:
			msgType = "WARNING";
			std::cout << termcolor::yellow;
			break;

		case T_ERROR:
			msgType = "ERROR";
			std::cout << termcolor::red;
			break;

		default: break;
		}

		std::cout << "[" << msgType << " @ " << caller << "]: " << message << ((newline) ? "\n" : "") << termcolor::reset;
	}


	/* Custom RTE exception class that allows for origin specification. */
	class RuntimeException : public std::exception {
	public:
		RuntimeException(const std::string& functionName, const std::string& message) : funcName(functionName), exceptionMessage(message) {}

		/* Gets the name of the origin from which the exception was raised. 
		* @return The name of the origin.
		*/
		inline const char* origin() const noexcept {
			return funcName.c_str();
		}

		/* Gets the error message. 
		* @return The error message.
		*/
		inline const char* what() const noexcept override {
			return exceptionMessage.c_str();
		}
	
	private:
		std::string funcName = "unknown origin";
		std::string exceptionMessage;
	};
}