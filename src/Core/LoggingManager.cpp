#include "LoggingManager.hpp"

namespace Log {
	std::deque<LogMessage> LogBuffer;
	size_t maxLogLines = 500;
}