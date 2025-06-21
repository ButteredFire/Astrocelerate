#include "LoggingManager.hpp"

namespace Log {
	std::deque<LogMessage> LogBuffer;
	int MaxLogLines = 500;
}