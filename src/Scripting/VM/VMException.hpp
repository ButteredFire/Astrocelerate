#pragma once

#include <format>
#include <string>
#include <exception>

#define VM_ASSERT_CONFIG(cond, excMsg, ...) \
	if (!(cond)) throw VMConfigException(excMsg, ##__VA_ARGS__);

#define VM_ASSERT_RUNTIME(cond, excMsg, ...) \
	if (!(cond)) throw VMRuntimeException(excMsg, ##__VA_ARGS__);


/* Virtual Machine Configuration Exception */
class VMConfigException : public std::exception {
public:
	template<typename... Args>
	VMConfigException(const std::string_view messageFmt, Args&&... args) :
		m_excMsg(std::vformat(messageFmt, std::make_format_args(args...))) {}

	~VMConfigException() = default;

	inline const char *what() const noexcept override {
		return m_excMsg.c_str();
	}

private:
	std::string m_excMsg;
};


/* Virtual Machine Runtime Exception */
class VMRuntimeException : public std::exception {
public:
	template<typename... Args>
	VMRuntimeException(const std::string_view messageFmt, Args&&... args) :
		m_excMsg(std::vformat(messageFmt, std::make_format_args(args...))) {}

	~VMRuntimeException() = default;

	inline const char *what() const noexcept override {
		return m_excMsg.c_str();
	}

private:
	std::string m_excMsg;
};


/* Virtual Machine Breakpoint Trap */
class VMBreakpointTrap : public std::exception {
public:
	VMBreakpointTrap(size_t progCounter) : m_pc(progCounter) {}
	~VMBreakpointTrap() = default;

	inline const char* what() const noexcept override {
		return std::format("Breakpoint triggered at instruction address 0x{:0>{}X}",
			m_pc - 1, 4
		).c_str();
	}

private:
	size_t m_pc;
};


/* Virtual Machine Trap upon Execution Finish at Simulation Tick */
class VMFinishedExecTick : public std::exception {
public:
	VMFinishedExecTick() {}
	~VMFinishedExecTick() = default;

	inline const char* what() const noexcept override {
		return "";
	}
};
